#pragma once

#include "Graphics/GraphicsEnums.h"
#include "Lib/Storm.h"
#include "Math/Primitives.h"

class CTexture;

struct IDirect3DTexture9;
struct IDirect3DSurface9;

class CGxTex;

class CGxTexCache {
public:
    using CacheGrid = TSExplicitList<CGxTexCache>[6][6];
    inline static CacheGrid& TSL_array = *reinterpret_cast<CacheGrid*>(0x00B49CD8);

    CGxTex* gx_tex_;
    uint32_t timestamp_;
    uint32_t size_;
    TSLink<CGxTexCache> link_;
};

static_assert(sizeof(CGxTexCache) == 0x14);

class CGxTexAtlasPage {
public:
    uint16_t ref_count_;
    uint16_t is_dirty_;
    uint32_t tile_width_;
    uint32_t tile_height_;
    uint32_t max_slots_;
    uint32_t num_slots_;
    uint32_t free_atlas_slot_index_;  // index into textures
    CGxTex* gx_tex_;
    GxTexFormat format_;
    GxTexFlags flags_;
    GxTexFormat data_format_;
    uint32_t max_textures_;
    CTexture* textures_[64];
    uint32_t updating_atlas_slot_index_;  // index into textures
    TSLink<CGxTexAtlasPage> link_;
};

static_assert(sizeof(CGxTexAtlasPage) == 0x138);

class CGxTex {
public:
    enum ProcOp : int32_t { eCalcSize = 0, eGenerate = 1, eFinish = 2, eRecreate = 3 };

    using ProcFunc = int(__cdecl*)(ProcOp op, int32_t width, int32_t height, int32_t face, int32_t mip_level,
        int32_t param, uint32_t* out_pitch, void** out_pixels);

    Recti dirty_rect_;
    Vec2i16 dirty_z_;

    int32_t width_;
    int32_t height_;
    int32_t depth_;
    int32_t target_;
    GxTexFormat format_;
    GxTexFormat data_format_;

    GxTexFlags flags_;

    int32_t callback_param_;
    ProcFunc callback_;

    IDirect3DTexture9* d3d_tex_;
    IDirect3DSurface9* d3d_surf_;

    TSLink<CGxTex> link_;

    int32_t unk_48_;
    int32_t unk_4_c_;
    int32_t unk_50_;
    int32_t unk_54_;
    uint16_t unk_58_;

    uint8_t needs_update_;
    uint8_t needs_create_;
    uint8_t is_dirty_;
    uint8_t post_create_flag_;
    uint16_t pad_;

    // tex, x0, y0, x1, y1 (exclusive max, an empty rect dirties all), upload_now (slot 0 right away)
    HOOKKIT_HOOK_HANDLE(
        markDirty, 0x00681F20, hookkit::Conv::eCdecl, CGxTex*, CGxTex*, int32_t, int32_t, int32_t, int32_t, int);
};

static_assert(sizeof(CGxTex) == 0x60);
