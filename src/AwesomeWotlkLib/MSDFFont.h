#pragma once
#include "MSDF.h"
#include "MSDFCache.h"
#include <map>

class MSDFFont {
    friend class MSDFCache;
    friend class MSDFPregen;

private:
    struct AtlasPage {
        IDirect3DTexture9* texture = nullptr;
        int nextX = 0, nextY = 0;
        int rowHeight = 0;
        int g = 0;
        std::vector<uint32_t> codepoints;

        AtlasPage(int gutter) : nextX(gutter), nextY(gutter), g(gutter) {}
        ~AtlasPage() { if (texture) texture->Release(); }

        void Clear() {
            nextX = g;
            nextY = g;
            rowHeight = 0;
            codepoints.clear();
        }
    };

public:
    MSDFFont(FT_Face face, const FT_Byte* fontData, FT_Long dataSize, FT_Long faceIndex);
    ~MSDFFont();

    bool IsValid() const { return m_isValid; }

    AtlasPage* GetAtlasPage(size_t index) const;
    size_t GetAtlasPageCount() const { return m_atlasPages.size(); }
    size_t GetAtlasEvictionCount() const { return m_evictionCount; }

    const GlyphMetrics* GetGlyph(uint32_t codepoint);

    static MSDFFont* Get(FT_Face face);
    static void Register(FT_Face face, const FT_Byte* data, FT_Long size, FT_Long index);
    static void Unregister(FT_Face face);
    static void ClearAllCache();
    static void Shutdown();

private:
    bool CreateAtlasPage();
    bool UploadGlyphToAtlas(GlyphMetrics& metrics, uint32_t codepoint);
    bool GenerateMSDF(std::vector<uint8_t>& outData, uint32_t codepoint, int sdfW, int sdfH);

    msdfgen::FontHandle* CreateMSDFHandle(const FT_Byte* data, FT_Long size);

    FT_Face m_ftFace;
    msdfgen::FontHandle* m_msdfFont;
    uint32_t m_oldestPage, m_evictionCount;
    bool m_isValid;

    std::unique_ptr<MSDFCache> m_cache;
    std::vector<std::unique_ptr<AtlasPage>> m_atlasPages;

    ankerl::unordered_dense::map<uint32_t, GlyphMetrics> m_glyphPool;

    inline static ankerl::unordered_dense::map<FT_Face, std::unique_ptr<MSDFFont>> s_fontHandles;

    inline static thread_local VectorPool<float> m_msdfPool;
};