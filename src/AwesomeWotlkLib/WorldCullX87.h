#pragma once

#include <emmintrin.h>

#include <array>
#include <cstdint>
#include <span>

#include "include/Map/WorldScene.h"
#include "include/Math/Primitives.h"

#ifdef __clang__
#pragma clang fp contract(off)
#elifdef _MSC_VER
#pragma fp_contract(off)
#endif

namespace world_cull::x87 {
inline Vec3f transformPoint(const Vec3f& v, const Mat4f& mat) {
    const auto& m = mat.data;
    std::array<float, 3> out{};
    for (int c = 0; c < 3; ++c) {
        double t = static_cast<double>(m[8 + c]) * static_cast<double>(v.z);
        t += static_cast<double>(m[4 + c]) * static_cast<double>(v.y);
        t += static_cast<double>(v.x) * static_cast<double>(m[c]);
        t += static_cast<double>(m[12 + c]);
        out[c] = static_cast<float>(t);
    }
    return {out[0], out[1], out[2]};
}

inline void transformClip(float x, float y, float z, const Mat4f& mat, std::span<float, 4> out) {
    const auto& m = mat.data;
    const auto r = [](float f) { return static_cast<double>(f); };
    constexpr float kW = 1.0f;
    double t = r(m[12]) * r(kW);
    t += r(m[4]) * r(y);
    t += r(m[8]) * r(z);
    t += r(x) * r(m[0]);
    out[0] = static_cast<float>(t);
    for (int c = 1; c < 4; ++c) {
        t = r(m[12 + c]) * r(kW);
        t += r(m[4 + c]) * r(y);
        t += r(m[c]) * r(x);
        t += r(m[8 + c]) * r(z);
        out[c] = static_cast<float>(t);
    }
}

inline __m128d toFloat(const __m128d& v) { return _mm_cvtps_pd(_mm_cvtpd_ps(v)); }

inline void toVec(const __m128d& x, const __m128d& y, const __m128d& z, std::span<Vec3f, 2> out) {
    alignas(16) std::array<float, 4> px{};
    alignas(16) std::array<float, 4> py{};
    alignas(16) std::array<float, 4> pz{};
    _mm_store_ps(px.data(), _mm_cvtpd_ps(x));
    _mm_store_ps(py.data(), _mm_cvtpd_ps(y));
    _mm_store_ps(pz.data(), _mm_cvtpd_ps(z));
    out[0] = {px[0], py[0], pz[0]};
    out[1] = {px[1], py[1], pz[1]};
}

struct Lanes {
    std::array<std::array<__m128d, 3>, 4> rows;

    void broadcast(const Mat4f& mat) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 3; ++c) {
                rows[r][c] = _mm_set1_pd(mat.data[r * 4 + c]);
            }
        }
    }

    void transform2(const __m128d& x, const __m128d& y, const __m128d& z, std::span<__m128d, 3> out) const {
        for (int c = 0; c < 3; ++c) {
            __m128d t = _mm_mul_pd(rows[2][c], z);
            t = _mm_add_pd(t, _mm_mul_pd(rows[1][c], y));
            t = _mm_add_pd(t, _mm_mul_pd(x, rows[0][c]));
            t = _mm_add_pd(t, rows[3][c]);
            out[c] = toFloat(t);
        }
    }

    void project2(const __m128d& x, const __m128d& y, const __m128d& z, std::span<Vec3f, 2> out) const {
        std::array<__m128d, 3> t{};
        transform2(x, y, z, t);
        const __m128d inv = _mm_div_pd(_mm_set1_pd(1.0), t[2]);
        toVec(_mm_mul_pd(t[0], inv), _mm_mul_pd(inv, t[1]), t[2], out);
    }

    void project2InFront(const __m128d& x, const __m128d& y, const __m128d& z, std::span<Vec3f, 2> out) const {
        std::array<__m128d, 3> t{};
        transform2(x, y, z, t);
        const __m128d inv = _mm_div_pd(_mm_set1_pd(1.0), t[2]);
        const __m128d scale = _mm_cmplt_pd(_mm_setzero_pd(), inv);
        const __m128d px = _mm_or_pd(_mm_and_pd(scale, _mm_mul_pd(t[0], inv)), _mm_andnot_pd(scale, t[0]));
        const __m128d py = _mm_or_pd(_mm_and_pd(scale, _mm_mul_pd(inv, t[1])), _mm_andnot_pd(scale, t[1]));
        toVec(px, py, t[2], out);
    }

    [[nodiscard]]
    bool projectBox(const AaBox& box, double near_depth, std::span<double, 8> xs, std::span<double, 8> ys) const {
        const __m128d x_min_max = _mm_set_pd(box.max.x, box.min.x);
        const __m128d x_max_min = _mm_set_pd(box.min.x, box.max.x);
        for (int p = 0; p < 4; ++p) {
            const __m128d y = _mm_set1_pd((p & 1) != 0 ? box.max.y : box.min.y);
            const __m128d z = _mm_set1_pd((p & 2) != 0 ? box.max.z : box.min.z);
            std::array<__m128d, 3> l{};
            transform2((p & 1) != 0 ? x_max_min : x_min_max, y, z, l);
            if (_mm_movemask_pd(_mm_cmplt_pd(l[2], _mm_set1_pd(near_depth))) != 0) { return false; }
            const __m128d inv = _mm_div_pd(_mm_set1_pd(1.0), l[2]);
            _mm_storeu_pd(&xs[2 * p], _mm_mul_pd(l[0], inv));
            _mm_storeu_pd(&ys[2 * p], _mm_mul_pd(inv, l[1]));
        }
        return true;
    }
};

inline double planeSide(const Plane& p, const Vec3f& v) {
    double d = static_cast<double>(p.normal.y) * static_cast<double>(v.y);
    d += static_cast<double>(p.normal.z) * static_cast<double>(v.z);
    d += static_cast<double>(p.normal.x) * static_cast<double>(v.x);
    return d + static_cast<double>(p.distance);
}

inline float viewDepth(const Plane& plane, const Vec3f& v) {
    double sum = static_cast<double>(plane.normal.z) * static_cast<double>(v.z);
    sum = sum + static_cast<double>(plane.normal.y) * static_cast<double>(v.y);
    sum = sum + static_cast<double>(plane.normal.x) * static_cast<double>(v.x);
    sum = sum + static_cast<double>(plane.distance);
    return static_cast<float>(sum);
}

inline int32_t fistp(double value) { return _mm_cvtsd_si32(_mm_set_sd(value)); }

inline int32_t floorIndex(float scaled) {
    return fistp(static_cast<double>(scaled) - static_cast<double>(world_scene::kFloorBias));
}
}  // namespace world_cull::x87
