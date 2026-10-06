#pragma once

#include <ankerl/unordered_dense.h>
#include <ft2build.h>
#include <hookkit/accessor.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "D3D.h"
#include "MSDF.h"
#include "MSDFCache.h"
#include "MSDFUtils.h"
#include "MSDFValidator.h"

#include FT_FREETYPE_H

class MSDFFont {
    friend class MSDFCache;
    friend class MSDFPregen;
    friend class MSDFFaceSweep;

    struct AtlasPage final : CGxTexExt::Source {
        MSDFFont* owner;
        CGxTexExt* texture = nullptr;
        int next_x = 0;
        int next_y = 0;
        int row_height = 0;
        int gutter = 0;
        std::vector<uint32_t> codepoints;
        const uint8_t* upload = nullptr;
        uint32_t upload_pitch = 0;
        CGxTexExt::Region upload_rect;

        AtlasPage(MSDFFont* owner, int gutter) : owner(owner), next_x(gutter), next_y(gutter), gutter(gutter) {}

        ~AtlasPage() override { CGxTexExt::destroy(texture); }

        AtlasPage(const AtlasPage&) = delete;
        AtlasPage& operator=(const AtlasPage&) = delete;
        AtlasPage(AtlasPage&&) = delete;
        AtlasPage& operator=(AtlasPage&&) = delete;

        void clear() {
            next_x = gutter;
            next_y = gutter;
            row_height = 0;
            codepoints.clear();
        }

        const void* pixels(int32_t face, int32_t mip, uint32_t& pitch) override;

        void onRecreate() override { owner->evictPage(*this); }
    };

    static_assert(msdf::kAtlasFormat == D3DFMT_A8R8G8B8, "the atlas is an engine eGxTexArgb8888 texture");

    static std::vector<uint8_t> makeZeroRow() {
        return std::vector<uint8_t>(static_cast<size_t>(msdf::kAtlasSize) * 4);
    }
    static constexpr utils::Accessor<std::vector<uint8_t>, struct ZeroRowTag, &MSDFFont::makeZeroRow> kZeroRow{};

public:
    MSDFFont(FT_Face face, const FT_Byte* font_data, FT_Long data_size);
    ~MSDFFont();

    MSDFFont(const MSDFFont&) = delete;
    MSDFFont& operator=(const MSDFFont&) = delete;
    MSDFFont(MSDFFont&&) = delete;
    MSDFFont& operator=(MSDFFont&&) = delete;

    [[nodiscard]]
    bool isValid() const {
        return is_valid_;
    }

    [[nodiscard]]
    AtlasPage* getAtlasPage(size_t index) const;

    [[nodiscard]]
    size_t getAtlasPageCount() const {
        return atlas_pages_.size();
    }

    [[nodiscard]]
    size_t getAtlasEvictionCount() const {
        return eviction_count_;
    }

    [[nodiscard]]
    const msdf_validator::FaceMetrics& getFaceMetrics() const {
        return face_metrics_;
    }

    [[nodiscard]]
    const GlyphMetrics* findGlyph(uint32_t codepoint) const;
    const GlyphMetrics* getGlyph(uint32_t codepoint);
    void loadGlyphs(std::span<const uint32_t> codepoints);

    static MSDFFont* get(FT_Face face);
    static void registerFace(FT_Face face, const FT_Byte* data, FT_Long size);
    static void unregisterFace(FT_Face face);
    static void clearAllCache();
    static void shutdown();

    static constexpr size_t kGlyphPoolReserve = 4096;
    static constexpr int kMaxSdfDim = 512;

private:
    bool createAtlasPage();
    void evictPage(AtlasPage& page);
    GlyphMetrics* uploadGlyphToAtlas(GlyphMetrics& metrics, uint32_t codepoint);
    static void generateMsdf(std::vector<uint8_t>& out_data, const msdfgen::Shape& shape,
        const msdfgen::Projection& projection, double scale, int sdf_w, int sdf_h);
    bool generateGlyphData(uint32_t codepoint, GlyphMetricsToStore& out_storage) const;

    static msdfgen::FontHandle* createMsdfHandle(const FT_Byte* data, FT_Long size);
    static MSDFCache::FaceRecord evaluateFace(msdfgen::FontHandle* font);

    msdfgen::FontHandle* msdf_font_ = nullptr;
    double em_size_ = 0.0;
    msdf_validator::FaceMetrics face_metrics_;
    bool is_valid_ = false;
    uint16_t oldest_page_ = 0;
    uint32_t eviction_count_ = 0;

    const FT_Byte* font_data_;
    FT_Long data_size_;

    std::unique_ptr<MSDFCache> cache_;
    std::vector<std::unique_ptr<AtlasPage>> atlas_pages_;

    ankerl::unordered_dense::map<uint32_t, GlyphMetrics> glyph_pool_;

    using FontHandleMap = ankerl::unordered_dense::map<FT_Face, std::unique_ptr<MSDFFont>>;

    static FontHandleMap makeFontHandles() { return {}; }
    struct FontHandlesTag;
    static constexpr utils::Accessor<FontHandleMap, FontHandlesTag, &MSDFFont::makeFontHandles> kFontHandles{};

    inline static thread_local VectorPool<float> msdf_pool_;
};
