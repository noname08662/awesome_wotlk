#include "MSDF.h"

#include <d3d9.h>
#include <ft2build.h>
#include <hookkit/trampoline.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "D3D.h"
#include "Extensions.h"
#include "MSDFFaceSweep.h"
#include "MSDFFont.h"
#include "MSDFPregen.h"
#include "MSDFShaders.h"
#include "MSDFValidator.h"

#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "include/Client/Client.h"
#include "include/Graphics/CGxDevice.h"
#include "include/Graphics/CGxFont.h"
#include "include/Graphics/CGxShader.h"
#include "include/Graphics/CGxString.h"
#include "include/Graphics/CGxStringBatch.h"
#include "include/Graphics/GraphicsEnums.h"
#include "include/Graphics/GxuFontUtil.h"
#include "include/Lib/FreeType.h"
#include "include/Lib/Lua.h"

namespace {
enum class Mode : int {
    eDisabled = 0,
    eEnabled = 1,
    eEnabledUnsafe = 2,
};

constexpr int32_t kMinStreamVerts = 0x800;
constexpr int32_t kMaxStreamVerts = 0xFFFC;
constexpr uint32_t kMaxFontQuads = 0x3FFF;  // 4 verts each, the last vertex index still fits the 16-bit index buffer
constexpr uint32_t kFontIndexCount = kMaxFontQuads * 6;
constexpr uint32_t kFontIndexPoolBytes = kFontIndexCount * sizeof(uint16_t);

constexpr uint32_t kFirstAtlasSampler = 16 - msdf::kMaxAtlasPages;  // the last d3d9 samplers, s12-s15 in MSDFShaders.h
constexpr size_t kEngineAtlasPages = std::extent_v<decltype(CGxFont::atlas_pages_)>;
constexpr size_t kPrefetchReserve = 16383;
constexpr int32_t kMonoBaselineSentinel = 0x40000000;
constexpr std::array<float, 4> kResetControl{};
constexpr msdf_validator::FaceMetrics kResetMetrics{};

constexpr CGxShaderExt::Name kFontShader{"MSDFFont"};

utils::Accessor<std::vector<uint32_t>, struct PrefetchPayloadTag> prefetch_payload;
utils::Accessor<std::vector<uint8_t>, struct MonoBitmapTag> mono_bitmap;

// MONOCHROME fonts are left to the engine's bitmap path, so they never resolve to an MSDF font
MSDFFont* msdfFontOf(const CGxFont* gxfont) {
    if (gxfont == nullptr || gxfont->hface_ == nullptr || gxfont->flags_.monochrome != 0) { return nullptr; }
    return MSDFFont::get(gxfont->hface_->getFontFace());
}

MSDFFont* msdfFontOf(const CGxString* str) { return msdfFontOf(str->gxfont_); }

// the px an outlined glyph sits into its padded engine cell
int outlineMode(CGxFont::Flags flags) {
    if (flags.thick_outline != 0) { return 2; }
    return flags.outline != 0 ? 1 : 0;
}

// the engine only ORs/tests bits 0-7 of texture_page_mask_
class CGxStringExt : public CGxString {
public:
    static constexpr uint32_t kEngineBits = 0xFF;
    static constexpr uint32_t kPending = 1u << 8;
    static constexpr uint32_t kTokenShift = 16;
    static constexpr uint32_t kTokenMask = 0xFFu << kTokenShift;
    static constexpr uint32_t kTokenPresent = 0x80;

    static CGxStringExt* of(CGxString* str) { return reinterpret_cast<CGxStringExt*>(str); }

    [[nodiscard]]
    bool pending() const {
        return (texture_page_mask_ & kPending) != 0;
    }

    void setPending() { texture_page_mask_ |= kPending; }

    void clearPending() { texture_page_mask_ &= ~kPending; }

    void setAtlasToken(uint32_t eviction_count) {
        texture_page_mask_ = (texture_page_mask_ & ~kTokenMask) | (token(eviction_count) << kTokenShift);
    }

    void clearAtlasToken() { texture_page_mask_ &= ~kTokenMask; }

    [[nodiscard]]
    bool needsAtlasRebuild(uint32_t eviction_count) const {
        const uint32_t stored = (texture_page_mask_ & kTokenMask) >> kTokenShift;
        return (stored & kTokenPresent) != 0 && stored != token(eviction_count);
    }

    void resetState() { texture_page_mask_ &= kEngineBits; }

    template <bool OutlinePass>
    bool splitsOutlines(const MSDFFont* font) const {
        if constexpr (!OutlinePass) {
            return false;
        } else {
            return font != nullptr && !this->hasFlag(CGxString::eGxStringFlag3D);
        }
    }

    template <bool OutlinePass>
    uint32_t pageVertCount(const MSDFFont* font, int page) {
        const uint32_t count = this->getVertCountForPage(page);
        return this->splitsOutlines<OutlinePass>(font) ? count * 2 : count;
    }

    // getOrCreateGlyphEntry_hook stores 1 + codepoint in every msdf glyph's u0, 0 = not a glyph
    static uint32_t glyphCodepoint(const VERT& vert) {
        return vert.u > 1.0f ? static_cast<uint32_t>(vert.u - 1.0f) : 0;
    }

    // verts the engine rebuilt since the last processGeometry, u still holds the codepoints
    // all msdf glyphs are on page 0
    [[nodiscard]]
    std::span<VERT> pendingVerts() const {
        const CGxString::TEXTLINETEXTURE* line = this->lines_[0];
        if (!this->pending() || line == nullptr || line->verts.count < 4) { return {}; }
        return {line->verts.data, line->verts.count};
    }

    void processGeometry(MSDFFont* font, std::vector<uint32_t>* misses) {
        const std::span<VERT> verts = this->pendingVerts();
        if (font == nullptr || verts.empty()) { return; }

        CGxFont* gxfont = this->gxfont_;
        const CGxFont::Flags flags = gxfont->flags_;
        const bool is_3d = this->hasFlag(CGxString::eGxStringFlag3D);  // native 3d obj - nameplate text, etc.
        const double font_size_mult = this->font_size_mult_;
        // outlined glyphs sit 1 (2 thick) px into their padded engine cell
        const double outline_offs = is_3d ? 0.0 : outlineMode(flags);
        const double baseline_offs = outline_offs > 0.0 ? 1.0 : 0.0;
        const double effective_height = is_3d
            ? font_size_mult
            : font_util::getFontEffectiveHeight{}(static_cast<int>(is_3d), this->font_size_mult_);
        const double scale = effective_height / msdf::kSdfRenderSize;
        const double pad = msdf::kSdfSpread * scale;
        // the engine's raster is capped at 32px and stretched to the effective height
        const double raster_scale =
            gxfont->raster_pixel_height_ > 0 ? effective_height / gxfont->raster_pixel_height_ : 1.0;

        const size_t evictions = font->getAtlasEvictionCount();
        bool complete = true;
        for (size_t q = 0; q < verts.size(); q += 4) {
            const std::span<VERT> quad = verts.subspan(q, 4);
            const uint32_t codepoint = glyphCodepoint(quad[0]);
            if (codepoint == 0) { continue; }

            const GlyphMetrics* gm = misses != nullptr ? font->findGlyph(codepoint) : font->getGlyph(codepoint);
            if (gm == nullptr) {
                if (misses != nullptr) {
                    misses->push_back(codepoint);
                    complete = false;
                }
                continue;
            }

            CGxFont::CHARCODEDESC* entry = gxfont->getOrCreateGlyphEntry(codepoint);
            if (entry == nullptr) { continue; }

            VERT& left_bottom = quad[0];
            VERT& left_top = quad[1];
            VERT& right_bottom = quad[2];
            VERT& right_top = quad[3];

            const double column0 = std::floor(entry->metrics.hori_bearing_x) * raster_scale;
            const double left = static_cast<double>(left_bottom.pos.x) + gm->bitmap_left * scale - column0 - pad +
                outline_offs * raster_scale;
            const double right = left + gm->width * scale;
            const double top = static_cast<double>(left_top.pos.y) + gm->bitmap_top * scale + pad - baseline_offs;
            const double bottom = top - gm->height * scale;

            left_bottom.pos.x = static_cast<float>(left);
            left_bottom.pos.y = static_cast<float>(bottom);
            left_top.pos.x = static_cast<float>(left);
            left_top.pos.y = static_cast<float>(top);
            right_bottom.pos.x = static_cast<float>(right);
            right_bottom.pos.y = static_cast<float>(bottom);
            right_top.pos.x = static_cast<float>(right);
            right_top.pos.y = static_cast<float>(top);

            // encode target msdf atlas page idx into the sign bits preserving the mantissa part bit-perfect
            const float u_sign = (gm->atlas_page_index & 1) != 0 ? -1.0f : 1.0f;
            const float v_sign = (gm->atlas_page_index & 2) != 0 ? -1.0f : 1.0f;

            left_bottom.u = gm->u0 * u_sign;
            left_bottom.v = gm->v0 * v_sign;
            left_top.u = gm->u0 * u_sign;
            left_top.v = gm->v1 * v_sign;
            right_bottom.u = gm->u1 * u_sign;
            right_bottom.v = gm->v0 * v_sign;
            right_top.u = gm->u1 * u_sign;
            right_top.v = gm->v1 * v_sign;
        }
        // store eviction count to later force engine to re-calc geometry when msdf page gets evicted
        this->setAtlasToken(evictions);
        if (complete) { this->clearPending(); }
    }

    // splits the page into core and outline regions (plus their shadow copies) so the outlines draw as a separate pass
    void writeOutlinePasses(std::span<VertOut> out, int index, uint32_t vert_index) {
        struct Region {
            int end;
            int offset;
        };

        int page_count = static_cast<int>(this->getVertCountForPage(index));
        std::array<Region, 4> regions{};
        size_t region_count;
        if (this->hasFlag(CGxString::eGxStringFlagShadow)) {
            page_count /= 2;
            regions = {{
                {.end = page_count, .offset = 0},
                {.end = 2 * page_count, .offset = -page_count},
                {.end = 3 * page_count, .offset = -page_count},
                {.end = 4 * page_count, .offset = -2 * page_count}
            }};
            region_count = 4;
        } else {
            regions[0] = {.end = page_count, .offset = 0};
            regions[1] = {.end = 2 * page_count, .offset = -page_count};
            region_count = 2;
        }

        size_t written = 0;
        auto current = static_cast<int>(vert_index);
        auto remaining = static_cast<int>(out.size());
        for (size_t i = 0; i < region_count && remaining > 0; ++i) {
            const auto [end, offset] = regions[i];
            if (current >= end) { continue; }

            const int count = std::min(remaining, end - current);
            const std::span<VertOut> chunk = out.subspan(written, static_cast<size_t>(count));
            this->writeGeometry(
                chunk.data(), index, static_cast<uint32_t>(current + offset), static_cast<uint32_t>(count));
            if ((i & 1) == 0) {
                // core pass sentinel, engine assigns all ui verts static z of 1.0f
                for (VertOut& vert : chunk) {
                    vert.z = -vert.z;
                }
            }
            written += static_cast<size_t>(count);
            current += count;
            remaining -= count;
        }
    }

private:
    static constexpr uint32_t token(uint32_t eviction_count) { return (eviction_count & 0x7F) | kTokenPresent; }
};

static_assert(sizeof(CGxStringExt) == sizeof(CGxString));
}  // namespace

namespace {
FT_Error renderMonoBitmap(FT_GlyphSlot slot) {
    FT_BBox cbox;
    FT_Outline_Get_CBox(&slot->outline, &cbox);
    const FT_Pos x_min = cbox.xMin & ~63;
    const FT_Pos y_min = cbox.yMin & ~63;
    const FT_Pos x_max = (cbox.xMax + 63) & ~63;
    const FT_Pos y_max = (cbox.yMax + 63) & ~63;

    FT_Bitmap& bitmap = slot->bitmap;
    bitmap.width = static_cast<unsigned int>((x_max - x_min) >> 6);
    bitmap.rows = static_cast<unsigned int>((y_max - y_min) >> 6);
    bitmap.pitch = static_cast<int>((bitmap.width + 7) >> 3);
    bitmap.pixel_mode = FT_PIXEL_MODE_MONO;
    bitmap.num_grays = 2;
    bitmap.buffer = nullptr;
    slot->bitmap_left = static_cast<FT_Int>(x_min >> 6);
    slot->bitmap_top = static_cast<FT_Int>(y_max >> 6);

    if (const size_t size = static_cast<size_t>(bitmap.pitch) * bitmap.rows; size != 0) {
        mono_bitmap->assign(size, 0);
        bitmap.buffer = mono_bitmap->data();
        FT_Outline_Translate(&slot->outline, -x_min, -y_min);
        const FT_Error error = FT_Outline_Get_Bitmap(slot->library, &slot->outline, &bitmap);
        FT_Outline_Translate(&slot->outline, x_min, y_min);
        if (error != 0) { return error; }
    }
    slot->format = FT_GLYPH_FORMAT_BITMAP;
    return 0;
}

HOOKKIT_BIND(freetype::init_hook, [](void*, FT_Library* alibrary) {
    if (const FT_Error error = FT_Init_FreeType(alibrary); error != 0) { return error; }

    msdf::msdf_freetype = msdfgen::initializeFreetype();
    if (msdf::msdf_freetype == nullptr) {
        FT_Done_FreeType(freetype::ft_library);
        freetype::ft_library = nullptr;
        return -1;
    }
    MSDFFaceSweep::start();
    return 0;
});

HOOKKIT_BIND(freetype::newMemoryFace_hook,
    [](FT_Library library, const FT_Byte* file_base, FT_Long file_size, FT_Long face_index, FT_Face* aface) {
        if (freetype::ft_library == nullptr && FT_Init_FreeType(&freetype::ft_library) != 0) { return -1; }

        const int result = FT_New_Memory_Face(library, file_base, file_size, face_index, aface);
        if (result != 0 || aface == nullptr || *aface == nullptr) { return result; }

        MSDFFont::registerFace(*aface, file_base, file_size);
        return result;
    });

HOOKKIT_BIND(freetype::setPixelSizes_hook,
    [](FT_Face face, FT_UInt width, FT_UInt height) { return FT_Set_Pixel_Sizes(face, width, height); });

// crashes with no FT_LOAD_RENDER, so the 1bpp bitmap is rendered right here
HOOKKIT_BIND(freetype::loadGlyph_hook, [](FT_Face face, FT_ULong glyph_index, FT_Int32 load_flags) {
    const auto index = static_cast<FT_UInt>(glyph_index);
    if (load_flags != freetype::kLoadFlagsMono) {
        return FT_Load_Glyph(face, index,
            FT_LOAD_RENDER | FT_LOAD_LINEAR_DESIGN | FT_LOAD_PEDANTIC | FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING);
    }

    FT_Error error = FT_Load_Glyph(face, index, freetype::kLoadFlagsMono | FT_LOAD_MONOCHROME | FT_LOAD_TARGET_MONO);
    if (error != 0) { return error; }

    // embedded bitmap strikes come in already rendered
    FT_GlyphSlot slot = face->glyph;
    if (slot->format == FT_GLYPH_FORMAT_OUTLINE && renderMonoBitmap(slot) != 0) {
        error = FT_Render_Glyph(slot, FT_RENDER_MODE_MONO);
        if (error != 0) { return error; }
        // this box can start at floor(xMin) + 1, re-anchor the bearing to its first column
        slot->metrics.horiBearingX = static_cast<FT_Pos>(slot->bitmap_left) * 64 + (slot->metrics.horiBearingX & 63);
    }
    return error;
});

HOOKKIT_BIND(
    freetype::getCharIndex_hook, [](FT_Face face, FT_ULong charcode) { return FT_Get_Char_Index(face, charcode); });

HOOKKIT_BIND(freetype::getKerning_hook,
    [](FT_Face face, FT_UInt left_glyph, FT_UInt right_glyph, FT_UInt kern_mode, FT_Vector* akerning) {
        return FT_Get_Kerning(face, left_glyph, right_glyph, kern_mode, akerning);
    });

HOOKKIT_BIND(freetype::doneFace_hook, [](FT_Face face) {
    MSDFFont::unregisterFace(face);
    return FT_Done_Face(face);
});

HOOKKIT_BIND(freetype::doneFreeType_hook, [](FT_Library) {
    MSDFFaceSweep::stop();
    MSDFFont::shutdown();
    if (msdf::msdf_freetype != nullptr) {
        msdfgen::deinitializeFreetype(msdf::msdf_freetype);
        msdf::msdf_freetype = nullptr;
    }
    if (freetype::ft_library != nullptr) {
        FT_Done_FreeType(freetype::ft_library);
        freetype::ft_library = nullptr;
    }
    return 0;
});

// ../Fonts/* init at startup, falls back to newMemoryFace
HOOKKIT_BIND(freetype::newFace_hook, [](int*, int) { return 1; });
}  // namespace

namespace {
void setFontShaderState(const std::array<float, 4>& control, const msdf_validator::FaceMetrics& metrics) {
    CGxDeviceD3dExt* gx = CGxDeviceD3dExt::of();
    if (gx == nullptr) { return; }
    gx->setConstants(eGxShaderPixel, msdf::kSdfSamplerSlot, control);
    gx->setConstants(eGxShaderVertex, msdf::kSdfSamplerSlot, control);

    constexpr auto kRenderSize = static_cast<float>(msdf::kSdfRenderSize);
    const std::array metric_constants{
        metrics.stem * kRenderSize, metrics.hairline * kRenderSize, metrics.counter * kRenderSize, kRenderSize};
    gx->setConstants(eGxShaderPixel, msdf::kSdfMetricsSlot, metric_constants);
}

void setStringShaderState(const CGxString* str, const MSDFFont* font) {
    const bool is_3d = str->hasFlag(CGxString::eGxStringFlag3D);
    const CGxFont::Flags flags = str->gxfont_->flags_;
    const float outline_mode = is_3d ? 0.0f : static_cast<float>(outlineMode(flags));
    constexpr auto kAtlasSize = static_cast<float>(msdf::kAtlasSize);
    constexpr auto kSdfSpread = static_cast<float>(msdf::kSdfSpread);
    setFontShaderState({is_3d ? 1.0f : 0.0f, outline_mode, kSdfSpread, kAtlasSize}, font->getFaceMetrics());
}

// the atlas pages only depend on the font, and a BATCHEDRENDERFONTDESC holds exactly one
void bindAtlasPages(CGxDeviceD3dExt* gx, const MSDFFont* font) {
    IDirect3DDevice9* device = gx->d3d();
    for (size_t page_index = 0; page_index < font->getAtlasPageCount(); ++page_index) {
        const auto* page = font->getAtlasPage(page_index);
        if (page == nullptr || page->texture == nullptr) { continue; }

        IDirect3DTexture9* texture = page->texture->realize();
        if (texture == nullptr) { continue; }
        const auto slot = kFirstAtlasSampler + page_index;
        device->SetTexture(slot, texture);
        gx->dsSet(static_cast<GxD3dDsState>(eGxDsSampAddressu0 + slot), D3DTADDRESS_CLAMP);
        gx->dsSet(static_cast<GxD3dDsState>(eGxDsSampAddressv0 + slot), D3DTADDRESS_CLAMP);
        gx->dsSet(static_cast<GxD3dDsState>(eGxDsSampMinfilter0 + slot), D3DTEXF_LINEAR);
        gx->dsSet(static_cast<GxD3dDsState>(eGxDsSampMagfilter0 + slot), D3DTEXF_LINEAR);
        gx->dsSet(static_cast<GxD3dDsState>(eGxDsSampMipfilter0 + slot), D3DTEXF_NONE);
    }
}
}  // namespace

namespace {
HOOKKIT_BIND(CGxString::checkGeometry_hook, [](CGxString* str) {
    CGxStringExt* self = CGxStringExt::of(str);
    if (MSDFFont* font = msdfFontOf(self); font != nullptr) {
        // force re-calc geometry if the MSDF page was evicted
        if (self->needsAtlasRebuild(font->getAtlasEvictionCount())) {
            self->clearInstanceData();
            self->clearAtlasToken();
        }
    }
    return self->checkGeometry();
});

HOOKKIT_BIND(CGxString::initTextLine_hook,
    [](CGxString* str, char* text, int text_length, Vec4u8* color, Vec3f* start_pos, uint32_t* pages_mask,
        CGxString::EMBEDDEDPARSEINFO* tex_info) {
        CGxStringExt* self = CGxStringExt::of(str);
        const int result = self->initTextLine(text, text_length, color, start_pos, pages_mask, tex_info);
        self->setPending();
        return result;
    });

// the engine reassigns flags_ here but never resets texture_page_mask_, so clear our state for the string's new life
HOOKKIT_BIND(CGxString::setValues_hook,
    [](CGxString* self, float font_size, Vec3f* anchor_pos, float bbox_width, float bbox_height, CGxFont* font,
        const char* text, CGxString::VertAlign vert_align, CGxString::HorzAlign horz_align, float line_spacing,
        uint32_t flags, Vec4u8* color, float font_scale) {
        const int result = self->setValues(font_size, anchor_pos, bbox_width, bbox_height, font, text, vert_align,
            horz_align, line_spacing, flags, color, font_scale);
        CGxStringExt::of(self)->resetState();
        return result;
    });

HOOKKIT_BIND(font_util::initGlyph_hook,
    [](FT_Face face, uint32_t font_size, uint32_t codepoint, uint32_t baseline, GLYPHBITMAPDATA* out,
        uint32_t monochrome, uint32_t pad) {
        // tag the baseline so glyphBearingY_site can tell monochrome glyphs (face-level state can't)
        const uint32_t tagged = monochrome != 0 ? baseline | static_cast<uint32_t>(kMonoBaselineSentinel) : baseline;
        const bool result = font_util::initGlyph{}(face, font_size, codepoint, tagged, out, monochrome, pad);
        if (monochrome == 0 && MSDFFont::get(face) != nullptr) {
            // ver_adv is distance from the top of the quad to the top of the glyph, bearing_y is the quad's vert
            // correction (descenders)
            out->bearing_y -= out->ver_adv;
        }
        return result;
    });

HOOKKIT_BIND(CGxFont::getOrCreateGlyphEntry_hook, [](CGxFont* self, uint32_t codepoint) {
    CGxFont::CHARCODEDESC* entry = self->getOrCreateGlyphEntry(codepoint);
    if (entry != nullptr && msdfFontOf(self) != nullptr) {
        entry->metrics.u0 = 1.0f + static_cast<float>(codepoint);  // store codepoint
        // everything on page 0: the engine draws page 0 of every string, then page 1, ..., so a glyph on a later
        // page would lay its shadow over its neighbours' contours
        entry->cell_index_min = 0;
        entry->cell_index_max = 0;
        entry->texture_page_index = 0;
    }
    return entry;
});

HOOKKIT_NAMED_BIND_RAW(allocateFontIndexBuffer_site, 0x006C480C, {"jmpback", 0x006C4811}) {
    constexpr uintptr_t kJmpback = allocateFontIndexBuffer_site::target("jmpback");
    return CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(
        jmpTo(kJmpback, movRegImm(Reg::eBx, kMaxFontQuads)));
};

// skip the engine's baseline (face->glyph->bitmap_top) for msdf faces, esi is the baseline here
// monochrome glyphs (tagged in initGlyph_hook) keep the engine's behavior; the tag is stripped on both paths
HOOKKIT_NAMED_BIND_RAW(glyphBearingY_site, 0x006C8C71, {"jmpback", 0x006C8C77}) {
    constexpr uintptr_t kJmpback = glyphBearingY_site::target("jmpback");
    constexpr int32_t kStripTag = ~kMonoBaselineSentinel;
    const uintptr_t skip_baseline = HOOKKIT_LAMBDA_ADDR([](FT_Face face, int32_t baseline) {
        return (baseline & kMonoBaselineSentinel) == 0 && MSDFFont::get(face) != nullptr;
    });
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .step(movRegMem(Reg::eDx, Reg::eCx, offsetof(FT_FaceRec, glyph)))
        .step(pushRegs(Reg::eAx, Reg::eDx))
        .step(pushReg(Reg::eSi))
        .step(pushReg(Reg::eCx))
        .build(skip_baseline,
            branchOnResult(ResultWidth::eByte, jmpTo(kJmpback, andReg(Reg::eSi, kStripTag), movRegImm(Reg::eCx, 0)),
                jmpTo(kJmpback, andReg(Reg::eSi, kStripTag),
                    movRegMem(Reg::eCx, Reg::eDx, offsetof(FT_GlyphSlotRec, bitmap_top))),
                popRegs(Reg::eAx, Reg::eDx)),
            8);
};

// all geometry is resolved before anything streams
template <bool OutlinePass>
void processBatch(CGxStringBatch::BATCHEDRENDERFONTDESC* desc) {
    if (CGxDevice::font_index_pool == nullptr) { return; }

    auto& strings = desc->strings_list;
    CGxFont* gxfont = desc->font;
    MSDFFont* font = msdfFontOf(gxfont);
    strings.enumerate([](CGxString* str) { CGxString::checkGeometry_hook::viaDetour(str); });

    // writes what's pooled and queues the rest
    std::vector<uint32_t>& misses = *prefetch_payload;
    strings.enumerate([&](CGxString* str) { CGxStringExt::of(str)->processGeometry(font, &misses); });
    if (!misses.empty()) {
        std::ranges::sort(misses);
        const auto duplicates = std::ranges::unique(misses);
        misses.erase(duplicates.begin(), duplicates.end());
        const size_t evictions = font->getAtlasEvictionCount();
        font->loadGlyphs(misses);
        misses.clear();
        if (font->getAtlasEvictionCount() != evictions) {
            strings.enumerate([](CGxString* str) { CGxString::checkGeometry_hook::viaDetour(str); });
        }
        strings.enumerate([font](CGxString* str) { CGxStringExt::of(str)->processGeometry(font, nullptr); });
    }

    for (CGxFont::TEXTURECACHE& page : gxfont->atlas_pages_) {
        page.markGxTexIfDirty();
    }

    std::array<CGxTex*, kEngineAtlasPages> page_textures{};
    int32_t max_page_verts = 0;
    for (size_t page = 0; page < kEngineAtlasPages; ++page) {
        CTexture* ctex = gxfont->atlas_pages_[page].ctex;
        if (ctex == nullptr) { continue; }
        page_textures[page] = CTexture::fetchGxTex{}(ctex, 1, nullptr);
        if (page_textures[page] == nullptr) { continue; }

        int32_t page_verts = 0;
        strings.enumerate([&](CGxString* str) {
            if (str->lines_[page] == nullptr) { return; }
            page_verts +=
                static_cast<int32_t>(CGxStringExt::of(str)->pageVertCount<OutlinePass>(font, static_cast<int>(page)));
        });
        max_page_verts = std::max(max_page_verts, page_verts);
    }

    // a page past kMaxStreamVerts still goes out, just across several draws
    const auto capacity = static_cast<uint32_t>(std::clamp(max_page_verts, kMinStreamVerts, kMaxStreamVerts));
    CGxDevice* device = CGxDevice::getDevice();
    CGxBuf* buf = device->bufStream(eGxPoolTargetVertex, sizeof(VertOut), capacity);
    std::span stream(static_cast<VertOut*>(device->bufLock(buf)), capacity);
    uint32_t written = 0;
    const auto flush = [&] {
        stream = {static_cast<VertOut*>(CGxDevice::flushBuffer{}(&buf, static_cast<int>(written))), capacity};
        written = 0;
    };

    CGxDeviceD3dExt* gx = CGxDeviceD3dExt::of();
    if (font == nullptr) {
        setFontShaderState(kResetControl, kResetMetrics);
    } else if (gx != nullptr && gx->d3d() != nullptr) {
        bindAtlasPages(gx, font);
    }

    for (size_t page = 0; page < kEngineAtlasPages; ++page) {
        if (page_textures[page] == nullptr) { continue; }
        device->rsSet(eGxRsTexture0, page_textures[page]);

        const auto page_index = static_cast<int>(page);
        strings.enumerate([&](CGxString* engine_str) {
            if (engine_str->lines_[page] == nullptr) { return; }
            CGxStringExt* str = CGxStringExt::of(engine_str);
            const uint32_t page_verts = str->pageVertCount<OutlinePass>(font, page_index);
            if (page_verts == 0) { return; }
            if (font != nullptr) { setStringShaderState(str, font); }

            const bool split_outlines = str->splitsOutlines<OutlinePass>(font);
            for (uint32_t vert_index = 0; vert_index < page_verts;) {
                const uint32_t count = std::min(capacity - written, page_verts - vert_index);
                const std::span<VertOut> chunk = stream.subspan(written, count);
                if (split_outlines) {
                    str->writeOutlinePasses(chunk, page_index, vert_index);
                } else {
                    str->writeGeometry(chunk.data(), page_index, vert_index, count);
                }
                vert_index += count;
                written += count;
                if (written == capacity) { flush(); }
            }
        });
        if (written != 0) { flush(); }
    }

    // balances the relock of the last flush, or the initial lock
    device->bufUnlock(buf, 0);
    buf->is_filled = 1;
}

void* processBatchDetour() {
    using Hook = CGxStringBatch::BATCHEDRENDERFONTDESC::processBatch_hook;
    return extensions::console::kCvarRegistry->get<"MSDFOutlinePass", int>() != 0
        ? Hook::staticDetour<processBatch<true>>()
        : Hook::staticDetour<processBatch<false>>();
}

void applyOutlinePass(int, bool changed) {
    using Hook = CGxStringBatch::BATCHEDRENDERFONTDESC::processBatch_hook;
    if (!changed || !Hook::attached.load(std::memory_order_acquire)) { return; }
    static_cast<void>(hookkit::HookTransaction::reinstall(Hook{}, processBatchDetour()));
    if (CGxShader* shader = CGxShader::font_pixel_shader) { CGxShaderExt::of(shader)->reload(); }
}
}  // namespace

namespace {
void applyFontShaders() {
    CGxShaderExt::replace({
        {.type = eGxShaderPixel, .name = kFontShader, .slots = {&CGxShader::font_pixel_shader, 1}},
        {.type = eGxShaderVertex, .name = kFontShader, .slots = {&CGxShader::font_vertex_shader, 1}}
    });
}

bool enableMsdf() {
    const std::string locale = client::getGameLocale();
    msdf::is_cjk = locale == "zhCN" || locale == "zhTW" || locale == "koKR";
    msdf::allow_unsafe_fonts =
        extensions::console::kCvarRegistry->get<"MSDFMode", int>() == static_cast<int>(Mode::eEnabledUnsafe);

    extensions::framescript::kOnEnter->add([] {
        lua::registerSlashCommand("MSDFPREGENCMD", "/msdfpregen", [](LuaState*) {
            MSDFPregen::tryStartPreGen();
            return 0;
        });
    });

    prefetch_payload->reserve(kPrefetchReserve);

    return true;
}

HOOKKIT_BIND(font_util::init_hook, []() {
    const int mode = extensions::console::kCvarRegistry->get<"MSDFMode", int>();
    if (mode == static_cast<int>(Mode::eDisabled)) {
        font_util::init{}();
        return;
    }

    CGxDevice* device = CGxDevice::getDevice();
    bool msdf_enabled = false;
    device->shaderCreate(&CGxShader::font_vertex_shader, eGxShaderVertex, "Shaders\\Vertex", "UI", 2);
    device->shaderCreate(&CGxShader::font_pixel_shader, eGxShaderPixel, "Shaders\\Pixel", "UI", 1);

    if (enableMsdf()) {
        CGxStringBatch::BATCHEDRENDERFONTDESC::processBatch_hook::raw_detour.store(processBatchDetour());
        if (hookkit::HookTransaction tx{}) {
            tx.attach(freetype::init_hook{}, freetype::newMemoryFace_hook{}, freetype::newFace_hook{},
                freetype::doneFace_hook{}, freetype::setPixelSizes_hook{}, freetype::getCharIndex_hook{},
                freetype::loadGlyph_hook{}, freetype::getKerning_hook{}, freetype::doneFreeType_hook{},
                font_util::initGlyph_hook{}, CGxFont::getOrCreateGlyphEntry_hook{}, allocateFontIndexBuffer_site{},
                glyphBearingY_site{}, CGxStringBatch::BATCHEDRENDERFONTDESC::processBatch_hook{},
                CGxString::checkGeometry_hook{}, CGxString::initTextLine_hook{}, CGxString::setValues_hook{});
            msdf_enabled = tx.commit() == NO_ERROR;
            if (msdf_enabled) { applyFontShaders(); }
        }
    }

    // CGxDevice::InitFontIndexBuffer, sized for a whole atlas page when msdf is on (see kMinStreamVerts)
    CGxDevice::font_index_pool = device->poolCreate(eGxPoolTargetIndex, eGxPoolUsageStatic,
        msdf_enabled ? kFontIndexPoolBytes : 0x1800, 0, "BATCHEDRENDERFONTDESC_idx");
    CGxDevice::font_index_buffer = CGxDevice::bufCreate{}(
        CGxDevice::font_index_pool, sizeof(uint16_t), msdf_enabled ? kFontIndexCount : 0xC00, nullptr);

    freetype::init::viaDetour(&freetype::ft_memory_interface, &freetype::ft_library);
    font_util::registerFontFaces{}(freetype::ft_library);
    font_util::onWindowsSizeChanged{}();
    font_util::createTextureQuads{}();
});
}  // namespace

void msdf::initialize(hookkit::HookTransaction& tx) {
    auto& cvars = *extensions::console::kCvarRegistry;
    cvars.add<int>({
        .name = "MSDFMode",
        .init = static_cast<int>(Mode::eEnabled),
        .min{static_cast<int>(Mode::eDisabled)},
        .max{static_cast<int>(Mode::eEnabledUnsafe)}
    });
    cvars.add<int>({.name = "MSDFOutlinePass", .init = 1, .min{0}, .max{1}, .on_change = applyOutlinePass});
    CGxShaderExt::define(eGxShaderPixel, kFontShader,
        {
            .hlsl = msdf_shaders::kPixelShaderHlsl,
            .defines = [] {
                return std::vector<CGxShaderExt::Define>{
                    {
                        .name = "OUTLINE_PASS",
                        .value = extensions::console::kCvarRegistry->get<"MSDFOutlinePass", int>() != 0 ? "1" : "0"
                    }
                };
            }
        });
    CGxShaderExt::define(eGxShaderVertex, kFontShader, {.hlsl = msdf_shaders::kVertexShaderHlsl});
    tx.attach(font_util::init_hook{});
}
