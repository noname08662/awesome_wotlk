#pragma once

#include <d3d9.h>
#include <ft2build.h>
#include <hookkit/transaction.h>
#include <msdfgen-ext.h>
#include <msdfgen.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include FT_FREETYPE_H

struct GlyphMetrics {
    uint16_t width = 0;
    uint16_t height = 0;
    FT_Int bitmap_top = 0;
    FT_Int bitmap_left = 0;
    float u0 = 0.0f, v0 = 0.0f, u1 = 0.0f, v1 = 0.0f;
    uint16_t atlas_page_index = 0;
    const uint8_t* pixel_data = nullptr;
};

struct GlyphMetricsToStore {
    uint32_t codepoint = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    FT_Int bitmap_top = 0;
    FT_Int bitmap_left = 0;
    std::vector<uint8_t> owned_pixel_data;
    uint32_t data_size = 0;
};

namespace msdf {
inline constexpr uint32_t kAtlasSize = 2048;                // 1024-2048
inline constexpr uint32_t kSdfSamplerSlot = 220;            // shader constant register, `c220` in MSDFShaders.h
inline constexpr uint32_t kSdfMetricsSlot = 221;            // per-face stroke metrics, `c221` in MSDFShaders.h
inline constexpr uint32_t kAtlasGutter = 12;                // usually spread + 2-4
inline constexpr uint32_t kSdfRenderSize = 64;              // 48-128
inline constexpr uint32_t kSdfSpread = 8;                   // 6-12
inline constexpr D3DFORMAT kAtlasFormat = D3DFMT_A8R8G8B8;  // D3DFMT_A8R8G8B8-D3DFMT_A16B16G16R16

inline constexpr uint32_t kMaxAtlasPages = 4;

inline msdfgen::FreetypeHandle* msdf_freetype = nullptr;

inline bool is_cjk = false;
// due to how distance fields are calculated, some fonts with self-intersecting contours (e.g. diediedie) will break
inline bool allow_unsafe_fonts = false;

void initialize(hookkit::HookTransaction& tx);
}  // namespace msdf
