#pragma once

#include <freetype/freetype.h>

#include <cstdint>

#include "Common/Common.h"
#include "Lib/Storm.h"
#include "Texture/CTexture.h"

class CGxFont;
class CGxString;

struct FONTHASHOBJ : CHandle {
    inline static auto& TSHT = *reinterpret_cast<TSHashTable<FONTHASHOBJ>*>(0x00B4A2A8);

    TSHashObject<FONTHASHOBJ> hash_obj;
    CGxFont* font;
};

static_assert(sizeof(FONTHASHOBJ) == 0x24);

struct VERT {
    Vec3f pos;
    float u, v;
};

static_assert(sizeof(VERT) == 0x14);

struct VertOut {
    float x, y, z;
    uint32_t color;
    float u, v;
};

static_assert(sizeof(VertOut) == 0x18);

struct FACEDATA : CHandle {
    inline static auto& TSHT = *reinterpret_cast<TSHashTable<FACEDATA>*>(0x00C7D2FC);

    TSHashObject<FACEDATA> hash_obj;
    void* sfile;
    FT_Face ft_face;
    CHandle* handle;

    FT_Face getFontFace() const { return ft_face; }
};

using Hface = FACEDATA*;
static_assert(sizeof(FACEDATA) == 0x2C);

struct GLYPHBITMAPDATA {
    void* pixel_data;
    uint32_t buffer_size;
    uint32_t width;
    uint32_t height;
    uint32_t cell_width;
    float advance_x;
    float hori_bearing_x;
    uint32_t pitch;
    uint32_t bearing_y;
    uint32_t ver_adv;
    float v1, u0, v0, u1;
};

static_assert(sizeof(GLYPHBITMAPDATA) == 0x38);

class CGxFont {
public:
    inline static auto& tsl = *reinterpret_cast<TSExplicitList<CGxFont>*>(0x00AD9AAC);

    struct CHARCODEDESC {
        TSHashObject<CHARCODEDESC> hash_obj;
        TSLink<CHARCODEDESC> tex_row_link;
        TSLink<CHARCODEDESC> glyph_link;
        uint32_t texture_page_index;
        uint32_t row_index;
        uint32_t cell_index_min;
        uint32_t cell_index_max;
        GLYPHBITMAPDATA metrics;
    };

    static_assert(sizeof(CHARCODEDESC) == 0x70);

    struct TEXTURECACHE {
        struct TEXTURECACHEROW {
            uint32_t remaining_width;
            TSExplicitList<CHARCODEDESC> glyphs_list;
        };

        CTexture* ctex;
        CGxFont* font;
        uint8_t is_dirty;
        char _pad[3];
        TSBaseArray<TEXTURECACHEROW> rows;

        HOOKKIT_HOOK(markGxTexIfDirty, 0x006C9D50, hookkit::Conv::eThiscall, void, TEXTURECACHE*);
    };

    static_assert(sizeof(TEXTURECACHE) == 0x18);

    struct KERNNODE {
        enum Flags : uint32_t {
            eHasHorizontal = 1u << 0,
            eHasVertical = 1u << 1,
        };

        TSHashObject<KERNNODE> hash_obj;
        Flags flags;
        float horz_kerning;
        float vert_kerning;
    };

    static_assert(sizeof(KERNNODE) == 0x24);

    struct Flags {
        uint32_t outline : 1;
        uint32_t monochrome : 1;
        uint32_t linear_filter : 1;  // bilinear atlas pages, 3d unit names
        uint32_t thick_outline : 1;
        uint32_t unk : 28;
    };

    TSLink<CGxFont> link_;
    TSExplicitList<CGxString> list_;
    TSHashTable<CHARCODEDESC> glyph_cache_;
    TSHashTable<KERNNODE> kern_cache_;
    TSExplicitList<CHARCODEDESC> glyph_list_;
    FACEDATA* hface_;
    char font_path_[260];
    int32_t atlas_pixel_size_;
    int32_t baseline_;
    Flags flags_;
    float font_height_;
    float font_scale_;
    TEXTURECACHE atlas_pages_[8];
    int raster_pixel_height_;

    HOOKKIT_HOOK(getOrCreateGlyphEntry, 0x006C3FC0, hookkit::Conv::eThiscall, CHARCODEDESC*, CGxFont*,
        uint32_t);  // codepoint
    HOOKKIT_HOOK(getBearingX, 0x006C24F0, hookkit::Conv::eThiscall, double, CGxFont*, CHARCODEDESC*, bool,
        float);  // is 3d, scale
    HOOKKIT_HOOK(reset, 0x006C3960, hookkit::Conv::eThiscall, int, CGxFont*);
};

static_assert(sizeof(CGxFont) == 0x250);
