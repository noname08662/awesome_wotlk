#pragma once

#include "Graphics/GraphicsEnums.h"
#include "Lib/Storm.h"
#include "Math/Primitives.h"
#include "Texture/CTexture.h"

class CRibbonEmitter {
public:
    enum Flags {
        eHasPrevTransform = 0x1,
        eDataEnabled = 0x4,
        eHasBone = 0x8,
        eEdgeTimingInitialized = 0x10,
    };

    struct Mat {
        uint32_t flags;
        GxBlend alpha;
    };

    static_assert(sizeof(Mat) == 0x8);

    struct Segment {
        Vec3f position0;
        ColorBGRA<> vertex_color0;
        float u0;
        float v0;
        Vec3f position1;
        ColorBGRA<> vertex_color1;
        float u1;
        float v1;
    };

    static_assert(sizeof(Segment) == 0x30);

    uint32_t ref_count_;
    TSGrowableArray<float> segment_time_pool_;
    uint32_t head_;
    uint32_t tail_;
    float spawn_accum_;
    Vec3f interp_position_;
    Vec3f position_;
    TSGrowableArray<Segment> segment_pool_;
    TSGrowableArray<uint16_t> edge_index_pool_;
    float oo_life_span_;
    float tmp_du_;
    float tmp_dv_;
    float oo_tmp_du_;
    float oo_tmp_dv_;
    float tex_v_top_;
    float tex_u_base_;
    float tex_v_bottom_;
    float tex_u_end_;
    Vec3f interp_up_;
    Vec3f history_up_;
    Vec3f interp_tangent_;
    Vec3f history_tangent_;
    Vec3f interp_tangent_offset_;
    Vec3f history_tangent_offset_;
    Vec3f interp_bottom_point_;
    Vec3f history_bottom_point_;
    Vec3f interp_top_point_;
    Vec3f history_top_point_;
    AaBox aa_box_;
    float edges_per_second_;
    float edge_life_span_;
    TSGrowableArray<Mat> materials_;
    TSGrowableArray<CTexture*> textures_;
    TSGrowableArray<uint32_t> replaces_;
    ColorBGRA<> diffuse_color_;
    float v0_;
    float u0_;
    float v1_;
    float u1_;
    uint32_t tex_rows_;
    uint32_t tex_cols_;
    Flags flags_;
    Vec3f history_position_;
    uint32_t tex_slot_;
    float above_;
    float below_;
    float gravity_;
};

static_assert(sizeof(CRibbonEmitter) == 0x180);
