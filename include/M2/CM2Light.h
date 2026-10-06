#pragma once

class CM2Scene;

class CM2Light {
public:
    CM2Scene* scene_;
    uint32_t tick_time_;
    uint32_t type_;
    Vec3f pos_;
    float float10_;
    float float14_;
    float float18_;
    Vec3f dir_;
    Vec3f amb_color_;
    Vec3f dir_color_;
    Vec3f spec_color_;
    float constant_attenuation_;
    float linear_attenuation_;
    float quadratic_attenuation_;
    uint32_t visible_;
    SlotLink<CM2Light> link_;
};

static_assert(sizeof(CM2Light) == 0x6C);
