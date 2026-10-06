#pragma once

#include "Math/Primitives.h"

namespace math {
inline Vec2f aspectNormal() {
    return {*reinterpret_cast<const float*>(0x00AC0CB4), *reinterpret_cast<const float*>(0x00AC0CB8)};
}

inline Vec2f ndcToDdc(Vec2f ndc) { return ndc * aspectNormal(); }

inline Vec2f ddcToNdc(Vec2f ddc) {
    const Vec2f aspect = aspectNormal();
    return {ddc.x / aspect.x, ddc.y / aspect.y};
}

// start, end, hit point, dist, flag, buffer
HOOKKIT_HOOK_HANDLE(
    traceLine, 0x007A3B70, hookkit::Conv::eCdecl, char, Vec3f*, Vec3f*, Vec3f*, float*, uint32_t, uint32_t);

static bool trace(
    Vec3f& start, Vec3f& end, uint32_t hit_flags, Vec3f& intersection_point, float& completed_before_intersection) {
    completed_before_intersection = 1.0f;
    intersection_point = Vec3f(0.0f, 0.0f, 0.0f);

    uint8_t result = traceLine{}(&start, &end, &intersection_point, &completed_before_intersection, hit_flags, 0);
    if (result != 0 && result != 1) { return false; }

    completed_before_intersection *= 100.0f;
    return static_cast<bool>(result);
}
};  // namespace math
