#pragma once

#include <emmintrin.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

#include "Math/Primitives.h"

#ifdef __clang__
#pragma clang fp contract(off)
#elifdef _MSC_VER
#pragma fp_contract(off)
#endif

namespace math {
inline bool pointInPolygon(const Vec3f& p, std::span<const Vec3f> verts, int axis) {
    constexpr int kAxisPairs[3][2] = {{1, 2}, {2, 0}, {0, 1}};
    const size_t count = verts.size();
    if (count == 0) { return false; }
    const int u = kAxisPairs[axis][0];
    const int v = kAxisPairs[axis][1];
    bool inside = false;
    bool y0 = p[v] <= verts[count - 1][v];
    for (size_t i = 0; i < count; ++i) {
        const Vec3f& cur = verts[i];
        const Vec3f& prev = verts[i == 0 ? count - 1 : i - 1];
        const bool y1 = p[v] <= cur[v];
        if (y0 != y1) {
            const double a = static_cast<double>(prev[u]) - static_cast<double>(cur[u]);
            const double b = static_cast<double>(cur[v]) - static_cast<double>(p[v]);
            const double ab = a * b;
            const double c = static_cast<double>(cur[u]) - static_cast<double>(p[u]);
            const double d = static_cast<double>(prev[v]) - static_cast<double>(cur[v]);
            const double cd = c * d;
            if ((cd <= ab) == y1) { inside = !inside; }
        }
        y0 = y1;
    }
    return inside;
}

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

// these match the engine's functions
namespace detail {
inline float toFloat(double v) { return static_cast<float>(v); }

inline __m128d toFloat(const __m128d& v) { return _mm_cvtps_pd(_mm_cvtpd_ps(v)); }

inline __m128d loadPair(const float* p) {
    return _mm_cvtps_pd(_mm_castsi128_ps(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(p))));
}

inline __m128 loadVec3(const Vec3f& v) {
    return _mm_movelh_ps(_mm_castsi128_ps(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(&v))), _mm_load_ss(&v.z));
}

inline Vec3f storeVec3(const __m128& v) {
    Vec3f out;
    _mm_storel_epi64(reinterpret_cast<__m128i*>(&out), _mm_castps_si128(v));
    _mm_store_ss(&out.z, _mm_movehl_ps(v, v));
    return out;
}

inline double lengthSq(const Vec3f& v) {
    const double x = v.x;
    const double y = v.y;
    const double z = v.z;
    return x * x + y * y + z * z;
}

inline Vec3f normalized(const Vec3f& v) {
    const double inv = 1.0 / std::sqrt(lengthSq(v));
    return {toFloat(v.x * inv), toFloat(v.y * inv), toFloat(v.z * inv)};
}

struct Elements {
    std::array<double, 16> d;

    explicit Elements(const Mat4f& m) {
        for (int i = 0; i < 16; ++i) {
            d[i] = m.data[i];
        }
    }

    [[nodiscard]]
    double t(int a, int b, int c) const {
        return d[a] * d[b] * d[c];
    }
};

inline float determinant(const Mat4f& m) {
    const Elements e{m};
    const auto t = [&e](int a, int b, int c) { return e.t(a, b, c); };
    const double c0 = t(6, 11, 13) + t(7, 9, 14) + t(5, 10, 15) - t(7, 10, 13) - t(6, 9, 15) - t(5, 11, 14);
    const double c1 = t(6, 11, 12) + t(7, 8, 14) + t(4, 10, 15) - t(7, 10, 12) - t(6, 8, 15) - t(4, 11, 14);
    const double c2 = t(5, 11, 12) + t(7, 8, 13) + t(4, 9, 15) - t(7, 9, 12) - t(5, 8, 15) - t(4, 11, 13);
    const double c3 = t(5, 10, 12) + t(6, 8, 13) + t(4, 9, 14) - t(6, 9, 12) - t(5, 8, 14) - t(4, 10, 13);
    return toFloat(c0 * e.d[0] - c1 * e.d[1] + c2 * e.d[2] - c3 * e.d[3]);
}

inline Mat4f adjoint(const Mat4f& m) {
    const Elements e{m};
    const auto t = [&e](int a, int b, int c) { return e.t(a, b, c); };
    Mat4f out;
    float* o = out.data;
    o[0] = toFloat(t(6, 11, 13) + t(7, 9, 14) + t(5, 10, 15) - t(7, 10, 13) - t(6, 9, 15) - t(5, 11, 14));
    o[1] = toFloat(-(t(2, 11, 13) + t(3, 9, 14) + t(1, 10, 15) - t(3, 10, 13) - t(2, 9, 15) - t(1, 11, 14)));
    o[2] = toFloat(t(1, 6, 15) + t(2, 7, 13) + t(3, 5, 14) - t(3, 6, 13) - t(2, 5, 15) - t(1, 7, 14));
    o[3] = toFloat(-(t(2, 7, 9) + t(3, 5, 10) + t(1, 6, 11) - t(3, 6, 9) - t(2, 5, 11) - t(1, 7, 10)));
    o[4] = toFloat(-(t(7, 8, 14) + t(4, 10, 15) + t(6, 11, 12) - t(7, 10, 12) - t(6, 8, 15) - t(4, 11, 14)));
    o[5] = toFloat(t(2, 11, 12) + t(3, 8, 14) + t(0, 10, 15) - t(3, 10, 12) - t(2, 8, 15) - t(0, 11, 14));
    o[6] = toFloat(-(t(2, 7, 12) + t(3, 4, 14) + t(0, 6, 15) - t(3, 6, 12) - t(2, 4, 15) - t(0, 7, 14)));
    o[7] = toFloat(t(2, 7, 8) + t(3, 4, 10) + t(0, 6, 11) - t(3, 6, 8) - t(2, 4, 11) - t(0, 7, 10));
    o[8] = toFloat(t(5, 11, 12) + t(7, 8, 13) + t(4, 9, 15) - t(7, 9, 12) - t(5, 8, 15) - t(4, 11, 13));
    o[9] = toFloat(-(t(1, 11, 12) + t(3, 8, 13) + t(0, 9, 15) - t(3, 9, 12) - t(1, 8, 15) - t(0, 11, 13)));
    o[10] = toFloat(t(1, 7, 12) + t(3, 4, 13) + t(0, 5, 15) - t(3, 5, 12) - t(1, 4, 15) - t(0, 7, 13));
    o[11] = toFloat(-(t(1, 7, 8) + t(3, 4, 9) + t(0, 5, 11) - t(3, 5, 8) - t(1, 4, 11) - t(0, 7, 9)));
    o[12] = toFloat(-(t(6, 8, 13) + t(4, 9, 14) + t(5, 10, 12) - t(6, 9, 12) - t(5, 8, 14) - t(4, 10, 13)));
    o[13] = toFloat(t(1, 10, 12) + t(2, 8, 13) + t(0, 9, 14) - t(2, 9, 12) - t(1, 8, 14) - t(0, 10, 13));
    o[14] = toFloat(-(t(1, 6, 12) + t(2, 4, 13) + t(0, 5, 14) - t(2, 5, 12) - t(1, 4, 14) - t(0, 6, 13)));
    o[15] = toFloat(t(1, 6, 8) + t(2, 4, 9) + t(0, 5, 10) - t(2, 5, 8) - t(1, 4, 10) - t(0, 6, 9));
    return out;
}

inline Mat4f inverse(const Mat4f& m) {
    const float scale = 1.0f / determinant(m);
    Mat4f out = adjoint(m);
    for (float& v : out.data) {
        v *= scale;
    }
    return out;
}

inline Vec3f frustumCorner(const Mat4f& m, double x, double y, double z, double w) {
    return {toFloat(m[3][0] * w + m[1][0] * y + m[2][0] * z + x * m[0][0]),
        toFloat(m[3][1] * w + m[1][1] * y + m[0][1] * x + m[2][1] * z),
        toFloat(m[3][2] * w + m[1][2] * y + m[0][2] * x + m[2][2] * z)};
}

inline constexpr std::array<std::array<uint8_t, 4>, 16> kMultiplyOrder = {{
    {2, 1, 3, 0},
    {2, 1, 0, 3},
    {1, 3, 0, 2},
    {2, 0, 1, 3},
    {1, 2, 3, 0},
    {1, 2, 3, 0},
    {3, 2, 1, 0},
    {1, 3, 2, 0},
    {1, 2, 3, 0},
    {1, 3, 2, 0},
    {3, 2, 1, 0},
    {1, 3, 2, 0},
    {1, 2, 3, 0},
    {1, 3, 2, 0},
    {3, 2, 1, 0},
    {1, 3, 2, 0},
}};
}  // namespace detail

inline Mat4f multiply(const Mat4f& lhs, const Mat4f& rhs) {
    const auto& l = lhs.data;
    const auto& r = rhs.data;
    Mat4f out;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            const auto& order = detail::kMultiplyOrder[i * 4 + j];
            double t = static_cast<double>(l[i * 4 + order[0]]) * static_cast<double>(r[order[0] * 4 + j]);
            for (int n = 1; n < 4; ++n) {
                t += static_cast<double>(l[i * 4 + order[n]]) * static_cast<double>(r[order[n] * 4 + j]);
            }
            out.data[i * 4 + j] = static_cast<float>(t);
        }
    }
    return out;
}

inline void translate(Mat4f& m, const Vec3f& move) {
    for (int c = 0; c < 3; ++c) {
        double t = static_cast<double>(m[2][c]) * static_cast<double>(move.z);
        t += static_cast<double>(m[1][c]) * static_cast<double>(move.y);
        t += static_cast<double>(move.x) * static_cast<double>(m[0][c]);
        t += static_cast<double>(m[3][c]);
        m[3][c] = static_cast<float>(t);
    }
}

// 0x007F9430
inline AaBox transformAaBox(const Mat4f& m, const AaBox& box) {
    __m128d lo = detail::loadPair(&m[3][0]);
    __m128d hi = lo;
    __m128d lo_z = _mm_cvtps_pd(_mm_load_ss(&m[3][2]));
    __m128d hi_z = lo_z;
    for (int r = 0; r < 3; ++r) {
        const __m128d box_lo = _mm_set1_pd(box.min[r]);
        const __m128d box_hi = _mm_set1_pd(box.max[r]);
        const __m128d row = detail::loadPair(&m[r][0]);
        const __m128d row_z = _mm_cvtps_pd(_mm_load_ss(&m[r][2]));

        const __m128d a = _mm_mul_pd(box_lo, row);
        const __m128d b = _mm_mul_pd(box_hi, row);
        lo = detail::toFloat(_mm_add_pd(lo, _mm_min_pd(a, b)));
        hi = detail::toFloat(_mm_add_pd(hi, _mm_max_pd(b, a)));

        const __m128d a_z = _mm_mul_sd(box_lo, row_z);
        const __m128d b_z = _mm_mul_sd(box_hi, row_z);
        lo_z = detail::toFloat(_mm_add_sd(lo_z, _mm_min_sd(a_z, b_z)));
        hi_z = detail::toFloat(_mm_add_sd(hi_z, _mm_max_sd(b_z, a_z)));
    }
    return {detail::storeVec3(_mm_movelh_ps(_mm_cvtpd_ps(lo), _mm_cvtpd_ps(lo_z))),
        detail::storeVec3(_mm_movelh_ps(_mm_cvtpd_ps(hi), _mm_cvtpd_ps(hi_z)))};
}

// 0x004C2FC0
inline Mat4f affineInverse(const Mat4f& m) {
    Mat4f out;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            out[r][c] = m[c][r];
        }
    }
    const double tx = -static_cast<double>(m[3][0]);
    const double ty = -static_cast<double>(m[3][1]);
    const double tz = -static_cast<double>(m[3][2]);
    for (int r = 0; r < 3; ++r) {
        out[3][r] = detail::toFloat(m[r][0] * tx + (m[r][1] * ty + m[r][2] * tz));
    }
    out[3][3] = 1.0f;
    return out;
}

inline Vec3f transformPoint(const Vec3f& p, const Mat4f& m) {
    const __m128 x = _mm_mul_ps(_mm_set1_ps(p.x), _mm_loadu_ps(m[0]));
    const __m128 y = _mm_mul_ps(_mm_set1_ps(p.y), _mm_loadu_ps(m[1]));
    const __m128 z = _mm_mul_ps(_mm_set1_ps(p.z), _mm_loadu_ps(m[2]));
    return detail::storeVec3(_mm_add_ps(_mm_add_ps(_mm_add_ps(x, y), z), _mm_loadu_ps(m[3])));
}

// 0x00984930
inline AaBox getAaBox(std::span<const Vec3f> points) {
    if (points.empty()) { return {}; }
    __m128 lo = detail::loadVec3(points[0]);
    __m128 hi = lo;
    for (size_t i = 1; i < points.size(); ++i) {
        const __m128 p = detail::loadVec3(points[i]);
        lo = _mm_min_ps(p, lo);
        hi = _mm_max_ps(p, hi);
    }
    return {detail::storeVec3(lo), detail::storeVec3(hi)};
}

// 0x006BF6D0
inline std::array<Vec3f, 8> calcFrustumCorners(const Mat4f& view, const Mat4f& projection) {
    constexpr float kOrthoEpsilon = 0x1p-22f;
    const Mat4f m = multiply(detail::inverse(projection), detail::inverse(view));
    double n = 1.0;
    double f = 1.0;
    if (!(std::fabs(projection[3][3] - 1.0f) < kOrthoEpsilon)) {
        const double top = -static_cast<double>(projection[3][2]);
        const double m33 = projection[2][2];
        n = detail::toFloat(top / (m33 + 1.0));
        f = detail::toFloat(top / (m33 - 1.0));
    }
    return {detail::frustumCorner(m, -n, -n, -n, n), detail::frustumCorner(m, -n, n, -n, n),
        detail::frustumCorner(m, n, n, -n, n), detail::frustumCorner(m, n, -n, -n, n),
        detail::frustumCorner(m, -f, -f, f, f), detail::frustumCorner(m, -f, f, f, f),
        detail::frustumCorner(m, f, f, f, f), detail::frustumCorner(m, f, -f, f, f)};
}

// 0x006BFE60
inline Mat4f createLookAt(const Vec3f& eye, const Vec3f& target, const Vec3f& up) {
    constexpr float kMinLengthSq = 0.01f;
    Mat4f out = Mat4f::identity();
    Vec3f f = target - eye;
    if (kMinLengthSq > detail::toFloat(detail::lengthSq(f))) { return out; }
    if (kMinLengthSq > detail::toFloat(detail::lengthSq(up))) { return out; }
    f = detail::normalized(f);
    const double ux = up.x;
    const double uy = up.y;
    const double uz = up.z;
    const Vec3f s = detail::normalized({detail::toFloat(f.y * uz - f.z * uy), detail::toFloat(f.z * ux - f.x * uz),
        detail::toFloat(f.x * uy - f.y * ux)});
    const double sx = s.x;
    const double sy = s.y;
    const double sz = s.z;
    const Vec3f u = detail::normalized({detail::toFloat(sy * f.z - sz * f.y), detail::toFloat(sz * f.x - sx * f.z),
        detail::toFloat(sx * f.y - sy * f.x)});
    for (int r = 0; r < 3; ++r) {
        out[r][0] = s[r];
        out[r][1] = u[r];
        out[r][2] = f[r];
    }
    translate(out, {-eye.x, -eye.y, -eye.z});
    return out;
}

inline bool trace(
    Vec3f& start, Vec3f& end, uint32_t hit_flags, Vec3f& intersection_point, float& completed_before_intersection) {
    completed_before_intersection = 1.0f;
    intersection_point = Vec3f(0.0f, 0.0f, 0.0f);

    uint8_t result = traceLine{}(&start, &end, &intersection_point, &completed_before_intersection, hit_flags, 0);
    if (result != 0 && result != 1) { return false; }

    completed_before_intersection *= 100.0f;
    return static_cast<bool>(result);
}
}  // namespace math
