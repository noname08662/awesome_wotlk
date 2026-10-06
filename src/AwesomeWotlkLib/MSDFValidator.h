#pragma once

#include <msdfgen-ext.h>
#include <msdfgen.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "include/Constants.h"
#include "include/Math/Primitives.h"

namespace msdf_validator {
inline constexpr uint32_t kRevision = 1;

namespace detail {
inline constexpr double kFlattenEpsRatio = 0.0005;
inline constexpr double kMaxCoordRatio = 64.0;
inline constexpr size_t kMinContourSize = 3;
inline constexpr int kMaxCurveSamples = 10;
inline constexpr int kMaxFlattenDepth = 20;
inline constexpr uint32_t kFirstCodepoint = 32;
inline constexpr uint32_t kLastCodepoint = 126;

using Contour = std::vector<Vec2d>;
using Contours = std::vector<Contour>;

struct Limits {
    double tol;
    double max_coord;
};

struct SpanCrossing {
    double at;
    int dir;
};

struct Scratch {
    msdfgen::Shape shape;
    Contours contours;
    std::vector<SpanCrossing> crossings;
};

[[nodiscard]]
inline Vec2d toVec(const msdfgen::Point2& p) {
    return {p.x, p.y};
}

[[nodiscard]]
inline bool isDegenerate(const Vec2d& a, const Vec2d& b) {
    return (b - a).lengthSq() <= kEps9d * kEps9d;
}

[[nodiscard]]
inline double distPointToLine(const Vec2d& p, const Vec2d& a, const Vec2d& b) {
    const Vec2d v = b - a;
    const Vec2d w = p - a;
    const double c2 = v.lengthSq();

    if (c2 <= kEps9d) { return w.length(); }

    const double t = std::clamp(w.dot(v) / c2, 0.0, 1.0);
    return p.distance(a + v * t);
}

[[nodiscard]]
inline Vec2d midpoint(const Vec2d& a, const Vec2d& b) {
    return (a + b) * 0.5;
}

inline void flattenQuadratic(
    const Vec2d& p0, const Vec2d& p1, const Vec2d& p2, Contour& out, double tol, int depth = 0) {
    const Vec2d m = p0 * 0.25 + p1 * 0.5 + p2 * 0.25;

    if (depth > kMaxFlattenDepth || distPointToLine(m, p0, p2) <= tol) {
        out.push_back(p2);
        return;
    }

    const Vec2d p01 = midpoint(p0, p1);
    const Vec2d p12 = midpoint(p1, p2);
    const Vec2d p012 = midpoint(p01, p12);

    flattenQuadratic(p0, p01, p012, out, tol, depth + 1);
    flattenQuadratic(p012, p12, p2, out, tol, depth + 1);
}

inline void flattenCubic(
    const Vec2d& p0, const Vec2d& p1, const Vec2d& p2, const Vec2d& p3, Contour& out, double tol, int depth = 0) {
    const Vec2d m = (p0 + p1 * 3.0 + p2 * 3.0 + p3) * 0.125;

    if (depth > kMaxFlattenDepth || distPointToLine(m, p0, p3) <= tol) {
        out.push_back(p3);
        return;
    }

    const Vec2d p01 = midpoint(p0, p1);
    const Vec2d p12 = midpoint(p1, p2);
    const Vec2d p23 = midpoint(p2, p3);
    const Vec2d p012 = midpoint(p01, p12);
    const Vec2d p123 = midpoint(p12, p23);
    const Vec2d p0123 = midpoint(p012, p123);

    flattenCubic(p0, p01, p012, p0123, out, tol, depth + 1);
    flattenCubic(p0123, p123, p23, p3, out, tol, depth + 1);
}

[[nodiscard]]
inline int orient(const Vec2d& a, const Vec2d& b, const Vec2d& c) {
    const double v = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    return static_cast<int>(v > kEps9d) - static_cast<int>(v < -kEps9d);
}

[[nodiscard]]
inline bool onSegment(const Vec2d& a, const Vec2d& b, const Vec2d& p) {
    if (orient(a, b, p) != 0) { return false; }
    return std::min(a.x, b.x) - kEps9d <= p.x && p.x <= std::max(a.x, b.x) + kEps9d &&
        std::min(a.y, b.y) - kEps9d <= p.y && p.y <= std::max(a.y, b.y) + kEps9d;
}

[[nodiscard]]
inline bool segsIntersect(const Vec2d& a1, const Vec2d& a2, const Vec2d& b1, const Vec2d& b2) {
    const int o1 = orient(a1, a2, b1);
    const int o2 = orient(a1, a2, b2);
    const int o3 = orient(b1, b2, a1);
    const int o4 = orient(b1, b2, a2);

    if (o1 != o2 && o3 != o4) { return true; }

    if (o1 == 0 && onSegment(a1, a2, b1)) { return true; }
    if (o2 == 0 && onSegment(a1, a2, b2)) { return true; }
    if (o3 == 0 && onSegment(b1, b2, a1)) { return true; }
    return o4 == 0 && onSegment(b1, b2, a2);
}

[[nodiscard]]
inline bool sharesEndpoint(const Vec2d& a1, const Vec2d& a2, const Vec2d& b1, const Vec2d& b2) {
    return isDegenerate(a1, b1) || isDegenerate(a1, b2) || isDegenerate(a2, b1) || isDegenerate(a2, b2);
}

[[nodiscard]]
inline bool edgesCross(const Vec2d& a1, const Vec2d& a2, const Vec2d& b1, const Vec2d& b2) {
    return segsIntersect(a1, a2, b1, b2) && !sharesEndpoint(a1, a2, b1, b2);
}

[[nodiscard]]
inline bool hasSelfIntersections(const Contour& pts) {
    const size_t n = pts.size();
    if (n < 4) { return false; }

    for (size_t i = 0; i < n; ++i) {
        const Vec2d& a1 = pts[i];
        const Vec2d& a2 = pts[(i + 1) % n];
        if (isDegenerate(a1, a2)) { continue; }

        for (size_t j = i + 2; j < n; ++j) {
            if (i == 0 && j == n - 1) { continue; }

            const Vec2d& b1 = pts[j];
            const Vec2d& b2 = pts[(j + 1) % n];
            if (isDegenerate(b1, b2)) { continue; }

            if (edgesCross(a1, a2, b1, b2)) { return true; }
        }
    }
    return false;
}

[[nodiscard]]
inline bool hasCrossContourIntersections(std::span<const Contour> contours) {
    for (size_t ci = 0; ci < contours.size(); ++ci) {
        const Contour& cont_a = contours[ci];
        const size_t na = cont_a.size();

        for (size_t cj = ci + 1; cj < contours.size(); ++cj) {
            const Contour& cont_b = contours[cj];
            const size_t nb = cont_b.size();

            for (size_t i = 0; i < na; ++i) {
                const Vec2d& a1 = cont_a[i];
                const Vec2d& a2 = cont_a[(i + 1) % na];
                if (isDegenerate(a1, a2)) { continue; }

                for (size_t j = 0; j < nb; ++j) {
                    const Vec2d& b1 = cont_b[j];
                    const Vec2d& b2 = cont_b[(j + 1) % nb];
                    if (isDegenerate(b1, b2)) { continue; }

                    if (edgesCross(a1, a2, b1, b2)) { return true; }
                }
            }
        }
    }
    return false;
}

[[nodiscard]]
inline bool hasNonDegenerateEdge(const Contour& pts) {
    for (size_t i = 0; i < pts.size(); ++i) {
        if (!isDegenerate(pts[i], pts[(i + 1) % pts.size()])) { return true; }
    }
    return false;
}

[[nodiscard]]
inline bool isValidContour(const Contour& pts) {
    return pts.size() >= kMinContourSize && hasNonDegenerateEdge(pts) && !hasSelfIntersections(pts);
}

[[nodiscard]]
inline bool hasValidControlPoints(const msdfgen::Contour& contour, double max_coord) {
    const auto valid = [max_coord](const msdfgen::Point2& p) {
        return std::isfinite(p.x) && std::isfinite(p.y) && std::abs(p.x) <= max_coord && std::abs(p.y) <= max_coord;
    };
    for (const auto& edge : contour.edges) {
        const msdfgen::EdgeSegment* seg = edge;
        if (seg == nullptr) { return false; }
        switch (seg->type()) {
            case msdfgen::LinearSegment::EDGE_TYPE: {
                const auto* line = static_cast<const msdfgen::LinearSegment*>(seg);
                if (!std::ranges::all_of(line->p, valid)) { return false; }
                break;
            }
            case msdfgen::QuadraticSegment::EDGE_TYPE: {
                const auto* quad = static_cast<const msdfgen::QuadraticSegment*>(seg);
                if (!std::ranges::all_of(quad->p, valid)) { return false; }
                break;
            }
            case msdfgen::CubicSegment::EDGE_TYPE: {
                const auto* cubic = static_cast<const msdfgen::CubicSegment*>(seg);
                if (!std::ranges::all_of(cubic->p, valid)) { return false; }
                break;
            }
            default: {
                if (!valid(seg->point(0.0)) || !valid(seg->point(1.0))) { return false; }
                break;
            }
        }
    }
    return true;
}

inline void flattenContour(const msdfgen::Contour& contour, double tol, Contour& out) {
    out.clear();
    for (const auto& edge : contour.edges) {
        const msdfgen::EdgeSegment* seg = edge;
        if (out.empty()) { out.push_back(toVec(seg->point(0.0))); }

        switch (seg->type()) {
            case msdfgen::LinearSegment::EDGE_TYPE: {
                const auto* line = static_cast<const msdfgen::LinearSegment*>(seg);
                out.push_back(toVec(line->p[1]));
                break;
            }
            case msdfgen::QuadraticSegment::EDGE_TYPE: {
                const auto* quad = static_cast<const msdfgen::QuadraticSegment*>(seg);
                flattenQuadratic(toVec(quad->p[0]), toVec(quad->p[1]), toVec(quad->p[2]), out, tol);
                break;
            }
            case msdfgen::CubicSegment::EDGE_TYPE: {
                const auto* cubic = static_cast<const msdfgen::CubicSegment*>(seg);
                flattenCubic(toVec(cubic->p[0]), toVec(cubic->p[1]), toVec(cubic->p[2]), toVec(cubic->p[3]), out, tol);
                break;
            }
            default: {
                for (int j = 1; j <= kMaxCurveSamples; ++j) {
                    out.push_back(toVec(seg->point(static_cast<double>(j) / kMaxCurveSamples)));
                }
                break;
            }
        }
    }
}

[[nodiscard]]
inline bool isShapeValid(const msdfgen::Shape& shape, const Limits& limits, Contours& flat) {
    const size_t count = shape.contours.size();
    if (flat.size() < count) { flat.resize(count); }

    for (size_t i = 0; i < count; ++i) {
        const msdfgen::Contour& contour = shape.contours[i];
        if (contour.edges.empty() || !hasValidControlPoints(contour, limits.max_coord)) { return false; }

        flattenContour(contour, limits.tol, flat[i]);
        if (!isValidContour(flat[i])) { return false; }
    }
    return !hasCrossContourIntersections(std::span<const Contour>(flat).first(count));
}

enum class GlyphResult : uint8_t { eValid, eEmpty, eInvalid };

[[nodiscard]]
inline GlyphResult checkGlyph(
    msdfgen::FontHandle* font, msdfgen::GlyphIndex index, const Limits& limits, Scratch& scratch) {
    if (!msdfgen::loadGlyph(scratch.shape, font, index)) { return GlyphResult::eInvalid; }
    if (scratch.shape.contours.empty()) { return GlyphResult::eEmpty; }
    return isShapeValid(scratch.shape, limits, scratch.contours) ? GlyphResult::eValid : GlyphResult::eInvalid;
}

inline bool validateFont(msdfgen::FontHandle* font, std::vector<uint32_t>* out_failing_codepoints) {
    if (font == nullptr) { return false; }

    msdfgen::FontMetrics metrics{};
    if (!msdfgen::getFontMetrics(metrics, font) || !(metrics.emSize > 0.0)) { return false; }  // not scalable
    const Limits limits{.tol = kFlattenEpsRatio * metrics.emSize, .max_coord = kMaxCoordRatio * metrics.emSize};

    std::vector<std::pair<unsigned, GlyphResult>> checked;
    checked.reserve(kLastCodepoint - kFirstCodepoint + 1);
    Scratch scratch;

    bool all_valid = true;
    bool any_outline = false;
    for (uint32_t cp = kFirstCodepoint; cp <= kLastCodepoint; ++cp) {
        msdfgen::GlyphIndex index;
        if (!msdfgen::getGlyphIndex(index, font, cp)) { continue; }

        const auto it = std::ranges::find(checked, index.getIndex(), &std::pair<unsigned, GlyphResult>::first);
        const GlyphResult result = it != checked.end()
            ? it->second
            : checked.emplace_back(index.getIndex(), checkGlyph(font, index, limits, scratch)).second;

        any_outline |= result == GlyphResult::eValid;
        if (result != GlyphResult::eInvalid) { continue; }

        all_valid = false;
        if (out_failing_codepoints == nullptr) { return false; }
        out_failing_codepoints->push_back(cp);
    }
    return all_valid && any_outline;
}
}  // namespace detail

[[nodiscard]]
inline bool isFontMsdfCompatible(msdfgen::FontHandle* font) {
    return detail::validateFont(font, nullptr);
}

[[nodiscard]]
inline bool isFontMsdfCompatible(msdfgen::FontHandle* font, std::vector<uint32_t>& out_failing_codepoints) {
    return detail::validateFont(font, &out_failing_codepoints);
}

struct FaceMetrics {
    float stem = 0.0f;      // dominant vertical stem width
    float hairline = 0.0f;  // 5th percentile of all stroke widths, the thin strokes of high-contrast faces
    float counter = 0.0f;   // smallest enclosed counter of o/e/a (O/D/Q on caps-only faces), the first to close
    float x_height = 0.0f;  // cap height on caps-only faces
};

namespace detail {
struct Bounds {
    double x0, y0, x1, y1;
};

[[nodiscard]]
inline std::span<const Contour> loadFlattened(
    msdfgen::FontHandle* font, uint32_t codepoint, double tol, Scratch& scratch) {
    msdfgen::GlyphIndex index;
    if (!msdfgen::getGlyphIndex(index, font, codepoint) || !msdfgen::loadGlyph(scratch.shape, font, index)) {
        return {};
    }
    size_t count = 0;
    for (const msdfgen::Contour& contour : scratch.shape.contours) {
        if (contour.edges.empty()) { continue; }
        if (count == scratch.contours.size()) { scratch.contours.emplace_back(); }
        flattenContour(contour, tol, scratch.contours[count++]);
    }
    return std::span<const Contour>(scratch.contours).first(count);
}

[[nodiscard]]
inline Bounds boundsOf(std::span<const Contour> contours) {
    Bounds b{.x0 = 1e300, .y0 = 1e300, .x1 = -1e300, .y1 = -1e300};
    for (const Contour& contour : contours) {
        for (const Vec2d& p : contour) {
            b = {
                .x0 = std::min(b.x0, p.x),
                .y0 = std::min(b.y0, p.y),
                .x1 = std::max(b.x1, p.x),
                .y1 = std::max(b.y1, p.y)
            };
        }
    }
    return b;
}

inline void scanSpans(std::span<const Contour> contours, double coord, bool horizontal, std::vector<double>* ink,
    std::vector<double>* gaps, std::vector<SpanCrossing>& crossings) {
    crossings.clear();
    for (const Contour& contour : contours) {
        const size_t n = contour.size();
        for (size_t i = 0; i < n; ++i) {
            const Vec2d& a = contour[i];
            const Vec2d& b = contour[(i + 1) % n];
            const double ao = horizontal ? a.y : a.x;
            const double bo = horizontal ? b.y : b.x;
            if ((ao <= coord) == (bo <= coord)) { continue; }
            const double t = (coord - ao) / (bo - ao);
            const double aa = horizontal ? a.x : a.y;
            const double ba = horizontal ? b.x : b.y;
            crossings.push_back({.at = aa + t * (ba - aa), .dir = bo > ao ? 1 : -1});
        }
    }
    std::ranges::sort(crossings, {}, &SpanCrossing::at);

    int winding = 0;
    double run_start = 0.0;
    double last_end = 0.0;
    bool any_run = false;
    for (const auto& [at, dir] : crossings) {
        const int next = winding + dir;
        if (winding == 0 && next != 0) {
            run_start = at;
            if (any_run && gaps != nullptr) { gaps->push_back(at - last_end); }
        } else if (winding != 0 && next == 0) {
            if (ink != nullptr) { ink->push_back(at - run_start); }
            last_end = at;
            any_run = true;
        }
        winding = next;
    }
}

[[nodiscard]]
inline double percentile(std::span<double> values, double q) {
    if (values.empty()) { return 0.0; }
    const auto k = static_cast<std::ptrdiff_t>(q * static_cast<double>(values.size() - 1));
    std::ranges::nth_element(values, values.begin() + k);
    return values[static_cast<size_t>(k)];
}
}  // namespace detail

[[nodiscard]]
inline FaceMetrics measureFace(msdfgen::FontHandle* font) {
    using namespace detail;
    msdfgen::FontMetrics font_metrics{};
    if (font == nullptr || !msdfgen::getFontMetrics(font_metrics, font) || !(font_metrics.emSize > 0.0)) { return {}; }
    const double em = font_metrics.emSize;
    const double tol = kFlattenEpsRatio * em;

    msdfgen::GlyphIndex lower_x;
    msdfgen::GlyphIndex upper_x;
    const bool has_lower = msdfgen::getGlyphIndex(lower_x, font, 'x') &&
        (!msdfgen::getGlyphIndex(upper_x, font, 'X') || lower_x.getIndex() != upper_x.getIndex());

    Scratch scratch;
    const std::span<const Contour> reference = loadFlattened(font, has_lower ? 'x' : 'H', tol, scratch);
    if (reference.empty()) { return {}; }
    const double ref_height = boundsOf(reference).y1;
    if (!(ref_height > 0.0)) { return {}; }

    std::vector<double> stems;
    for (const char c : std::string_view(has_lower ? "nmhuil" : "HILTE")) {
        const std::span<const Contour> glyph = loadFlattened(font, static_cast<unsigned char>(c), tol, scratch);
        if (glyph.empty()) { continue; }
        for (const double f : {0.3, 0.5, 0.7}) {
            scanSpans(glyph, ref_height * f, true, &stems, nullptr, scratch.crossings);
        }
    }
    const double stem = percentile(stems, 0.5);

    std::vector<double> strokes = std::move(stems);
    for (const char c : std::string_view(has_lower ? "oenbdpq" : "OEBDPQ")) {
        const std::span<const Contour> glyph = loadFlattened(font, static_cast<unsigned char>(c), tol, scratch);
        if (glyph.empty()) { continue; }
        const Bounds b = boundsOf(glyph);
        for (const double f : {0.35, 0.5, 0.65}) {
            scanSpans(glyph, b.x0 + (b.x1 - b.x0) * f, false, &strokes, nullptr, scratch.crossings);
        }
    }

    std::vector<double> counters;
    std::vector<double> gaps;
    for (const char c : std::string_view(has_lower ? "oea" : "ODQ")) {
        const std::span<const Contour> glyph = loadFlattened(font, static_cast<unsigned char>(c), tol, scratch);
        if (glyph.empty()) { continue; }
        const auto [x0, y0, x1, y1] = boundsOf(glyph);
        gaps.clear();
        scanSpans(glyph, (y0 + y1) * 0.5, true, nullptr, &gaps, scratch.crossings);
        scanSpans(glyph, (x0 + x1) * 0.5, false, nullptr, &gaps, scratch.crossings);
        if (!gaps.empty()) { counters.push_back(*std::ranges::min_element(gaps)); }
    }

    return {
        .stem = static_cast<float>(stem / em),
        .hairline = static_cast<float>(percentile(strokes, 0.05) / em),
        .counter = static_cast<float>(percentile(counters, 0.0) / em),
        .x_height = static_cast<float>(ref_height / em),
    };
}
}  // namespace msdf_validator
