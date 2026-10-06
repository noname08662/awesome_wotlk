#pragma once

#include "M2/M2Data.h"

class CM2Scene;
class CM2Light;

class CM2Lighting {
public:
    enum CM2LightingFlags : uint32_t {
        eCameraSpaceReady = 0x1,
        eSunlightReady = 0x2,
        eInitialized = 0x20,
        eUnkBit06LiquidPlaneGate = 0x40,
    };

    CM2Scene* scene_;
    AaSphere bounds_;
    CM2LightingFlags flags_;
    Vec3f diffuse_dir_sum_r_;
    Vec3f diffuse_dir_sum_g_;
    Vec3f diffuse_dir_sum_b_;
    Vec3f diffuse_dir_sum_luma_;
    Vec3f diffuse_color_sum_;
    Vec3f sun_ambient_;
    Vec3f sun_diffuse_;
    Vec3f sun_specular_;
    Vec3f sun_dir_;
    CM2Light* lights_[4];
    float light_distances_sq_[4];
    uint32_t light_count_;
    float fog_start_;
    float fog_end_;
    float fog_scale_;
    float fog_density_;
    Vec3f fog_color_;
    Plane liquid_plane_;
};

static_assert(sizeof(CM2Lighting) == 0xD4);
