#pragma once

#include "Math/Primitives.h"

class CSimpleTexture;

struct CBackdropGenerator {
    // which edge/corner textures SetOutput (0x4A2E00) creates
    enum EdgeMask : uint32_t {
        eEdgeLeft = 0x1,
        eEdgeRight = 0x2,
        eEdgeTop = 0x4,
        eEdgeBottom = 0x8,
        eEdgeTopLeft = 0x10,
        eEdgeTopRight = 0x20,
        eEdgeBottomLeft = 0x40,
        eEdgeBottomRight = 0x80,
    };

    CSimpleTexture* bg_tex;
    CSimpleTexture* left_tex;
    CSimpleTexture* right_tex;
    CSimpleTexture* top_tex;
    CSimpleTexture* bottom_tex;
    CSimpleTexture* topleft_tex;
    CSimpleTexture* topright_tex;
    CSimpleTexture* bottomleft_tex;
    CSimpleTexture* bottomright_tex;
    unk_t _unk24[2];
    const char* bg_file;
    unk_t _unk30[2];
    const char* edge_file;
    EdgeMask edge_mask;
    uint32_t tile;
    unk_t _unk44;
    // sizes and insets are layout units (DDC before the owner's scale_), not UI units
    float edge_size;
    float tile_size;
    float inset_top;
    float inset_bottom;
    float inset_left;
    float inset_right;
    Vec4u8 bg_color;
    Vec4u8 border_color;
    uint32_t edge_blend_mode;
};

static_assert(sizeof(CBackdropGenerator) == 0x6C);
static_assert(offsetof(CBackdropGenerator, edge_size) == 0x48);
static_assert(offsetof(CBackdropGenerator, border_color) == 0x64);
