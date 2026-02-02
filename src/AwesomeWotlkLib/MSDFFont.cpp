#include "MSDFFont.h"
#include "MSDFCache.h"
#include "MSDFValidator.h"

MSDFFont::MSDFFont(FT_Face face, const FT_Byte* fontData, FT_Long dataSize, FT_Long faceIndex)
    : m_ftFace(face), m_msdfFont(nullptr), m_isValid(false), m_oldestPage(0), m_evictionCount(0)
{
    if (!face) return;

    m_msdfFont = CreateMSDFHandle(fontData, dataSize);
    if (!m_msdfFont) return;

    m_cache = std::make_unique<MSDFCache>(fontData, dataSize,
        face->family_name ? face->family_name : "Unknown", face->style_name ? face->style_name : "",
        MSDF::SDF_RENDER_SIZE, MSDF::SDF_SPREAD);

    if (MSDF::IS_CJK) {
        m_isValid = (m_cache->GetManifestSize() >= MSDF::CJK_CACHE_THRESHOLD) &&
            (MSDF::ALLOW_UNSAFE_FONTS || MSDFValidator::IsFontMSDFCompatible(m_msdfFont));
    }
    else {
        m_isValid = m_cache->GetManifestSize() || MSDF::ALLOW_UNSAFE_FONTS || MSDFValidator::IsFontMSDFCompatible(m_msdfFont);
    }
    if (m_isValid) m_glyphPool.reserve(4096);
}

MSDFFont::~MSDFFont() {
    m_glyphPool.clear();
    m_atlasPages.clear();
    m_cache.reset();
    if (m_msdfFont) {
        msdfgen::destroyFont(m_msdfFont);
        m_msdfFont = nullptr;
    }
}

MSDFFont* MSDFFont::Get(FT_Face face) {
    auto it = s_fontHandles.find(face);
    if (it != s_fontHandles.end() && it->second->IsValid()) {
        return it->second.get();
    }
    return nullptr;
}

void MSDFFont::Register(FT_Face face, const FT_Byte* data, FT_Long size, FT_Long index) {
    if (s_fontHandles.find(face) != s_fontHandles.end()) return;
    auto font = std::make_unique<MSDFFont>(face, data, size, index);
    if (font->m_msdfFont && (MSDF::IS_PREGEN || font->m_isValid)) s_fontHandles[face] = std::move(font);
}

void MSDFFont::Unregister(FT_Face face) {
    auto it = s_fontHandles.find(face);
    if (it != s_fontHandles.end()) {
        s_fontHandles.erase(it);
    }
}

void MSDFFont::ClearAllCache() {
    for (auto& [face, handle] : s_fontHandles) {
        if (handle) {
            handle->m_atlasPages.clear();
            handle->m_oldestPage = 0;
            handle->m_evictionCount = 0;
        }
    }
}

void MSDFFont::Shutdown() {
    s_fontHandles.clear();
}

const GlyphMetrics* MSDFFont::GetGlyph(uint32_t codepoint) {
    auto pit = m_glyphPool.find(codepoint);
    if (pit != m_glyphPool.end()) return &pit->second;

    auto [it, inserted] = m_glyphPool.try_emplace(codepoint);
    GlyphMetrics& metrics = it->second;

    if (m_cache->TryLoadGlyph(codepoint, metrics)) {
        UploadGlyphToAtlas(metrics, codepoint);
        return &metrics;
    }

    GlyphMetricsToStore storage;
    storage.codepoint = codepoint;

    if (FT_Set_Pixel_Sizes(m_ftFace, MSDF::SDF_RENDER_SIZE, MSDF::SDF_RENDER_SIZE) != 0) {
        m_glyphPool.erase(it);
        return nullptr;
    }

    FT_UInt glyphIndex = FT_Get_Char_Index(m_ftFace, codepoint);
    if (FT_Load_Glyph(m_ftFace, glyphIndex, FT_LOAD_NO_BITMAP | FT_LOAD_NO_HINTING) != 0) {
        m_glyphPool.erase(it);
        return nullptr;
    }

    storage.bitmapLeft = m_ftFace->glyph->bitmap_left;
    storage.bitmapTop = m_ftFace->glyph->bitmap_top;

    const bool hasOutline = m_ftFace->glyph->format == FT_GLYPH_FORMAT_OUTLINE &&
        m_ftFace->glyph->outline.n_contours > 0;
    if (hasOutline) {
        FT_BBox bbox;
        FT_Outline_Get_BBox(&m_ftFace->glyph->outline, &bbox);

        int w = std::max(0, static_cast<int>(((bbox.xMax + 63) >> 6) - (bbox.xMin >> 6)));
        int h = std::max(0, static_cast<int>(((bbox.yMax + 63) >> 6) - (bbox.yMin >> 6)));

        if (w > 0 && h > 0) {
            int sdfW = w + 2 * MSDF::SDF_SPREAD;
            int sdfH = h + 2 * MSDF::SDF_SPREAD;
            storage.ownedPixelData.reserve(static_cast<size_t>(sdfW) * sdfH * 4);
            if (GenerateMSDF(storage.ownedPixelData, codepoint, sdfW, sdfH)) {
                storage.width = sdfW;
                storage.height = sdfH;
                storage.dataSize = static_cast<uint32_t>(storage.ownedPixelData.size());
                metrics.width = storage.width;
                metrics.height = storage.height;
                metrics.bitmapLeft = storage.bitmapLeft;
                metrics.bitmapTop = storage.bitmapTop;
                metrics.pixelData = storage.ownedPixelData.data();
                UploadGlyphToAtlas(metrics, codepoint);
            }
        }
    }
    m_cache->StoreGlyph(std::move(storage));

    return &metrics;
}

MSDFFont::AtlasPage* MSDFFont::GetAtlasPage(size_t index) const {
    if (index < m_atlasPages.size()) {
        return m_atlasPages[index].get();
    }
    return nullptr;
}

bool MSDFFont::CreateAtlasPage() {
    auto page = std::make_unique<AtlasPage>(MSDF::ATLAS_GUTTER);
    if (!D3D::CreateTexture(&page->texture, {
        .width = MSDF::ATLAS_SIZE,
        .height = MSDF::ATLAS_SIZE,
        .format = MSDF::D3DFMT,
        .pool = D3DPOOL_MANAGED
        })) {
        return false;
    }
    m_atlasPages.push_back(std::move(page));
    return true;
}

bool MSDFFont::UploadGlyphToAtlas(GlyphMetrics& metrics, uint32_t codepoint) {
    if (!metrics.pixelData || metrics.width == 0 || metrics.height == 0) return true;

    int pageIndex = -1;
    AtlasPage* targetPage = nullptr;

    for (size_t i = 0; i < m_atlasPages.size(); ++i) {
        AtlasPage* page = m_atlasPages[i].get();
        if (page->nextX + metrics.width + MSDF::ATLAS_GUTTER <= MSDF::ATLAS_SIZE &&
            page->nextY + metrics.height + MSDF::ATLAS_GUTTER <= MSDF::ATLAS_SIZE) {
            pageIndex = static_cast<int>(i);
            targetPage = page;
            break;
        }
        int nextY = page->nextY + page->rowHeight + MSDF::ATLAS_GUTTER;
        if (nextY + metrics.height + MSDF::ATLAS_GUTTER <= MSDF::ATLAS_SIZE) {
            page->nextX = MSDF::ATLAS_GUTTER;
            page->nextY = nextY;
            page->rowHeight = 0;
            pageIndex = static_cast<int>(i);
            targetPage = page;
            break;
        }
    }
    if (pageIndex == -1) {
        if (m_atlasPages.size() >= MSDF::MAX_ATLAS_PAGES) {
            pageIndex = m_oldestPage;
            targetPage = m_atlasPages[m_oldestPage].get();

            for (uint32_t cp : targetPage->codepoints) {
                auto it = m_glyphPool.find(cp);
                if (it != m_glyphPool.end()) {
                    m_glyphPool.erase(it);
                }
            }
            targetPage->Clear();

            D3DLOCKED_RECT fullRect;
            if (SUCCEEDED(targetPage->texture->LockRect(0, &fullRect, nullptr, 0))) {
                memset(fullRect.pBits, 0, MSDF::ATLAS_SIZE * fullRect.Pitch);
                targetPage->texture->UnlockRect(0);
            }
            m_evictionCount++;
            m_oldestPage = (m_oldestPage + 1) % MSDF::MAX_ATLAS_PAGES;
        }
        else {
            if (!CreateAtlasPage()) return false;
            pageIndex = static_cast<int>(m_atlasPages.size() - 1);
            targetPage = m_atlasPages.back().get();
        }
    }
    if (!targetPage->texture) return false;

    D3DLOCKED_RECT lockedRect;
    if (FAILED(targetPage->texture->LockRect(0, &lockedRect, nullptr, 0))) {
        return false;
    }
    if (lockedRect.Pitch < metrics.width * 4) {
        targetPage->texture->UnlockRect(0);
        return false;
    }

    const unsigned char* src = metrics.pixelData;
    unsigned char* dest = static_cast<unsigned char*>(lockedRect.pBits) +
        targetPage->nextY * lockedRect.Pitch + targetPage->nextX * 4;
    for (int y = 0; y < metrics.height; ++y) {
        memcpy(dest, src, metrics.width * 4);
        dest += lockedRect.Pitch;
        src += metrics.width * 4;
    }
    targetPage->texture->UnlockRect(0);

    float atlasSize = static_cast<float>(MSDF::ATLAS_SIZE);
    metrics.u0 = static_cast<float>(targetPage->nextX) / atlasSize;
    metrics.v0 = static_cast<float>(targetPage->nextY) / atlasSize;
    metrics.u1 = static_cast<float>(targetPage->nextX + metrics.width) / atlasSize;
    metrics.v1 = static_cast<float>(targetPage->nextY + metrics.height) / atlasSize;
    metrics.atlasPageIndex = pageIndex;

    targetPage->nextX += metrics.width + MSDF::ATLAS_GUTTER;
    targetPage->rowHeight = std::max(targetPage->rowHeight, static_cast<int>(metrics.height));
    targetPage->codepoints.push_back(codepoint);

    return true;
}

bool MSDFFont::GenerateMSDF(std::vector<uint8_t>& outData, uint32_t codepoint, int sdfW, int sdfH) {
    if (sdfW <= 0 || sdfH <= 0 || sdfW > 512 || sdfH > 512) return false;

    msdfgen::Shape shape;
    if (!msdfgen::loadGlyph(shape, m_msdfFont, codepoint)) return false;

    if (shape.contours.empty()) {
        outData.assign(sdfW * sdfH * 4, 0);
        return true;
    }

    msdfgen::resolveShapeGeometry(shape);
    msdfgen::edgeColoringInkTrap(shape, 3.0, 0);

    auto bounds = shape.getBounds();
    double shapeW = bounds.r - bounds.l;
    double shapeH = bounds.t - bounds.b;
    if (shapeW <= 0 || shapeH <= 0) return false;

    double usableW = static_cast<double>(sdfW) - 2.0 * MSDF::SDF_SPREAD;
    double usableH = static_cast<double>(sdfH) - 2.0 * MSDF::SDF_SPREAD;
    if (usableW <= 0 || usableH <= 0) return false;

    double scale = std::min(usableW / shapeW, usableH / shapeH);
    msdfgen::Projection projection(
        msdfgen::Vector2(scale, scale),
        msdfgen::Vector2(MSDF::SDF_SPREAD / scale - bounds.l, MSDF::SDF_SPREAD / scale - bounds.b)
    );

    auto msdfBuf = m_msdfPool.AcquireSized(sdfW * sdfH * 3);
    auto sdfBuf = m_msdfPool.AcquireSized(sdfW * sdfH);

    msdfgen::BitmapRef<float, 3> msdfBitmap(msdfBuf.data(), sdfW, sdfH);
    msdfgen::BitmapRef<float, 1> sdfBitmap(sdfBuf.data(), sdfW, sdfH);

    msdfgen::MSDFGeneratorConfig config;
    config.overlapSupport = true;

    msdfgen::Range msdfRange(MSDF::SDF_SPREAD / scale);
    msdfgen::generateMSDF(msdfBitmap, shape, projection, msdfRange, config);
    msdfgen::SDFTransformation msdfTransform(projection, msdfRange);
    msdfgen::distanceSignCorrection(msdfBitmap, shape, msdfTransform, msdfgen::FillRule::FILL_NONZERO);

    msdfgen::Range sdfRange(MSDF::SDF_SPREAD / scale * 5.0);
    msdfgen::generateSDF(sdfBitmap, shape, projection, sdfRange);
    msdfgen::SDFTransformation sdfTransform(projection, sdfRange);
    msdfgen::distanceSignCorrection(sdfBitmap, shape, sdfTransform, msdfgen::FillRule::FILL_NONZERO);

    outData.resize(sdfW * sdfH * 4);
    uint8_t* dest = outData.data();
    const float* srcMSDF = msdfBuf.data();
    const float* srcSDF = sdfBuf.data();

    for (int i = 0; i < sdfW * sdfH; ++i) {
        dest[i * 4 + 0] = static_cast<uint8_t>(std::clamp(srcMSDF[i * 3 + 0] * 255.f, 0.f, 255.f));
        dest[i * 4 + 1] = static_cast<uint8_t>(std::clamp(srcMSDF[i * 3 + 1] * 255.f, 0.f, 255.f));
        dest[i * 4 + 2] = static_cast<uint8_t>(std::clamp(srcMSDF[i * 3 + 2] * 255.f, 0.f, 255.f));
        dest[i * 4 + 3] = static_cast<uint8_t>(std::clamp(srcSDF[i] * 255.f, 0.f, 255.f));
    }
    m_msdfPool.Release(std::move(msdfBuf));
    m_msdfPool.Release(std::move(sdfBuf));

    return true;
}

msdfgen::FontHandle* MSDFFont::CreateMSDFHandle(const FT_Byte* data, FT_Long size) {
    return !MSDF::g_msdfFreetype ? nullptr : msdfgen::loadFontData(MSDF::g_msdfFreetype, data, size);
}