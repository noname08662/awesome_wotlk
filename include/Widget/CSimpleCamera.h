#pragma once

#include "Math/Primitives.h"

class CSimpleCamera {
public:
    struct M2Scene {
        unk_t unk[82];
    };

    static_assert(sizeof(M2Scene) == 0x148);

    virtual double getFov();
    virtual Vec3f* getForwardVector(Vec3f* out);
    virtual Vec3f* getLeftVector(Vec3f* out);
    virtual Vec3f* getUpVector(Vec3f* out);

    M2Scene* scene_;   // 0x04
    Vec3f pos_;        // 0x08
    Mat3f matrix_;     // 0x14 (fwd, left, up)
    float near_clip_;  // 0x38
    float far_clip_;   // 0x3C
    float fov_;        // 0x40
    float aspect_;     // 0x44

    // near clip, far clip, fov
    HOOKKIT_HOOK(ctor, 0x00607C20, hookkit::Conv::eThiscall, CSimpleCamera*, CSimpleCamera*, float, float, float);
};
