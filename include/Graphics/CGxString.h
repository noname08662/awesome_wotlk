#pragma once

#include <freetype/freetype.h>

#include <cstddef>
#include <cstdint>

#include "Graphics/CGxFont.h"
#include "Lib/Storm.h"

class CGxString {
public:
    struct TEXTLINETEXTURE {
        inline static auto& TSL = *reinterpret_cast<TSExplicitList<TEXTLINETEXTURE>*>(0x00AD9BCC);
        inline static auto& cached_font_geom_bytes = *reinterpret_cast<uint32_t*>(0x00C7D2D8);

        TSLink<TEXTLINETEXTURE> link;
        TSGrowableArray<VERT> verts;
        TSGrowableArray<Vec4u8> colors;
    };

    static_assert(sizeof(TEXTLINETEXTURE) == 0x28);

    struct GRADIENTINFO {
        uint32_t page_index;
        uint32_t vertex_index;
    };

    static_assert(sizeof(GRADIENTINFO) == 0x8);

    struct GXUFONTHYPERLINKINFO {
        Rectf rect;
        const char* link_tag;
        uint32_t link_tag_length;
        const char* link_text;
        uint32_t link_text_length;
        const char* color_tag;
        uint32_t color_tag_length;
    };

    static_assert(sizeof(GXUFONTHYPERLINKINFO) == 0x28);

    struct GXUEMBEDDEDTEXTUREINFO {
        Rectf bounds;
        const char* texture_path;
        uint32_t texture_path_len;
        Vec2f offset;
        Vec2f dims;
        Rectf coords;
        int32_t has_custom_tex_coords;
    };

    static_assert(sizeof(GXUEMBEDDEDTEXTUREINFO) == 0x3C);

    struct EMBEDDEDPARSEINFO {
        enum ParseState : int32_t {
            eHyperlinkStateNone = 0,    // not inside a hyperlink tag
            eHyperlinkStateText = 1,    // inside an active hyperlink (|H...|h)
            eHyperlinkStateTexture = 2  // inside an active hyperlink, currently wrapping an embedded texture (|T...|t)
        };

        ParseState state;
        GXUFONTHYPERLINKINFO link_info;
        GXUEMBEDDEDTEXTUREINFO texture_tag;
    };

    static_assert(sizeof(EMBEDDEDPARSEINFO) == 0x68);

    inline static auto& TSL_active = *reinterpret_cast<TSExplicitList<CGxString>*>(0x00AD9AC4);
    inline static auto& TSL_free = *reinterpret_cast<TSExplicitList<CGxString>*>(0x00AD9AB8);
    inline static auto& loading_tip_str = *reinterpret_cast<CGxString*>(0x00B2FEE4);

    enum Flags : uint32_t {
        eGxStringFlagNone = 0x0000,
        eGxStringFlagShadow = 0x0001,
        eGxStringFlagSingleLine = 0x0002,
        eGxStringFlagWordWrap = 0x0004,
        eGxStringFlagNoColor = 0x0008,
        eGxStringFlagVertical = 0x0010,
        eGxStringFlagGradient = 0x0020,
        eGxStringFlagOutline = 0x0040,
        eGxStringFlag3D = 0x0080,
        eGxStringFlagHangingIndent = 0x2000,
    };

    enum HorzAlign : uint32_t {
        eGxHorzAlignLeft = 0,
        eGxHorzAlignCenter = 1,
        eGxHorzAlignRight = 2,
    };

    enum VertAlign : uint32_t {
        eGxVertAlignBottom = 0,
        eGxVertAlignMiddle = 1,
        eGxVertAlignTop = 2,
    };

    TSLink<CGxString> lifecycle_link_;
    TSLink<CGxString> font_link_;
    TSLink<CGxString> render_link_;
    float font_size_;
    float font_size_mult_;
    Vec3f anchor_pos_;
    Vec4u8 text_color_;
    Vec4u8 shadow_color_;
    Vec2f shadow_offset_;
    Vec2f bbox_;
    CGxFont* gxfont_;
    const char* text_;
    uint32_t text_capacity_;
    VertAlign vert_align_;
    HorzAlign horz_align_;
    float line_spacing_;
    uint32_t flags_;
    uint32_t texture_page_mask_;
    uint32_t is_dirty_;
    int32_t gradient_start_char_;
    int32_t gradient_length_;
    Vec3f final_pos_;
    float font_scale_;
    TSGrowableArray<GXUFONTHYPERLINKINFO> hyperlinks_;
    TSGrowableArray<GXUEMBEDDEDTEXTUREINFO> embedded_textures_;
    TSGrowableArray<GRADIENTINFO> gradient_info_;
    uint32_t num_visible_lines_;
    TEXTLINETEXTURE* lines_[8];
    uint32_t last_render_time_;

    bool hasFlag(Flags flag) const { return (flags_ & flag) == static_cast<uint32_t>(flag); }

    void setFlag(Flags flag) { flags_ |= flag; }

    void clearFlag(Flags flag) { flags_ &= ~flag; }

    FT_Face getFontFace() const { return reinterpret_cast<FT_Face (*)(FACEDATA*)>(0x006C8080)(this->gxfont_->hface_); }

    // dest, idx, vert idx, vert count
    HOOKKIT_HOOK(
        writeGeometry, 0x006C5E90, hookkit::Conv::eThiscall, void, CGxString*, VertOut*, int, uint32_t, uint32_t);
    // text, text length, text col, start pos, pages mask, tex info
    HOOKKIT_HOOK(initTextLine, 0x006C6CD0, hookkit::Conv::eThiscall, int, CGxString*, char*, int, Vec4u8*, Vec3f*,
        uint32_t*, EMBEDDEDPARSEINFO*);
    // font size, anchor pos, bbox width, bbox height, font, text, vert align, horz align, line spacing, flags, text col,
    // font scale
    HOOKKIT_HOOK(setValues, 0x006C74D0, hookkit::Conv::eThiscall, int, CGxString*, float, Vec3f*, float, float,
        CGxFont*, const char*, VertAlign, HorzAlign, float, uint32_t, Vec4u8*, float);
    HOOKKIT_HOOK(clearInstanceData, 0x006C6B90, hookkit::Conv::eThiscall, void, CGxString*);
    HOOKKIT_HOOK(checkGeometry, 0x006C7480, hookkit::Conv::eThiscall, bool, CGxString*);
    HOOKKIT_HOOK(getVertCountForPage, 0x006C63E0, hookkit::Conv::eThiscall, uint32_t, CGxString*, int);
};

static_assert(sizeof(CGxString) == 0xD8);
