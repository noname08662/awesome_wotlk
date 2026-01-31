#pragma once

class MSDFValidator {
public:
    static bool IsFontMSDFCompatible(msdfgen::FontHandle* font) {
        if (!font) return false;

        for (uint32_t cp = 32; cp < 127; ++cp) {
            if (!IsGlyphValid(font, cp)) {
                return false;
            }
        }
        return true;
    }
private:
    static constexpr double FLATTEN_EPS = 0.5;
    static constexpr double EPS = 1e-9;
    static constexpr double MAX_COORD = 1e9;
    static constexpr int MIN_CONTOUR_SIZE = 3;
    static constexpr int MAX_CURVE_SAMPLES = 10;

    static inline bool isnan_inf(double v) {
        return std::isnan(v) || std::isinf(v);
    }

    static inline bool isValidCoord(double v) {
        return !isnan_inf(v) && std::abs(v) <= MAX_COORD;
    }

    struct Vec {
        double x, y;

        Vec() : x(0), y(0) {}
        Vec(double x_, double y_) : x(x_), y(y_) {}

        inline double length() const { return std::hypot(x, y); }
        inline double lengthSq() const { return x * x + y * y; }
    };

    static double distPointToLine(const Vec& p, const Vec& a, const Vec& b) {
        const double vx = b.x - a.x;
        const double vy = b.y - a.y;
        const double wx = p.x - a.x;
        const double wy = p.y - a.y;
        const double c2 = vx * vx + vy * vy;

        if (c2 <= EPS) return std::hypot(wx, wy);

        const double t = std::clamp((wx * vx + wy * vy) / c2, 0.0, 1.0);
        const double px = a.x + t * vx;
        const double py = a.y + t * vy;

        return std::hypot(p.x - px, p.y - py);
    }

    static void flattenQuadratic(const Vec& p0, const Vec& p1, const Vec& p2,
        std::vector<Vec>& out, double tol, int depth = 0) {
        if (depth > 20) {
            out.push_back(p2);
            return;
        }

        const Vec m(0.25 * p0.x + 0.5 * p1.x + 0.25 * p2.x,
            0.25 * p0.y + 0.5 * p1.y + 0.25 * p2.y);

        if (distPointToLine(m, p0, p2) <= tol) {
            out.push_back(p2);
            return;
        }

        const Vec p01((p0.x + p1.x) * 0.5, (p0.y + p1.y) * 0.5);
        const Vec p12((p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5);
        const Vec p012((p01.x + p12.x) * 0.5, (p01.y + p12.y) * 0.5);

        flattenQuadratic(p0, p01, p012, out, tol, depth + 1);
        flattenQuadratic(p012, p12, p2, out, tol, depth + 1);
    }

    static void flattenCubic(const Vec& p0, const Vec& p1, const Vec& p2, const Vec& p3,
        std::vector<Vec>& out, double tol, int depth = 0) {
        if (depth > 20) {
            out.push_back(p3);
            return;
        }

        const Vec m(0.125 * (p0.x + 3.0 * p1.x + 3.0 * p2.x + p3.x),
            0.125 * (p0.y + 3.0 * p1.y + 3.0 * p2.y + p3.y));

        if (distPointToLine(m, p0, p3) <= tol) {
            out.push_back(p3);
            return;
        }

        const Vec p01((p0.x + p1.x) * 0.5, (p0.y + p1.y) * 0.5);
        const Vec p12((p1.x + p2.x) * 0.5, (p1.y + p2.y) * 0.5);
        const Vec p23((p2.x + p3.x) * 0.5, (p2.y + p3.y) * 0.5);
        const Vec p012((p01.x + p12.x) * 0.5, (p01.y + p12.y) * 0.5);
        const Vec p123((p12.x + p23.x) * 0.5, (p12.y + p23.y) * 0.5);
        const Vec p0123((p012.x + p123.x) * 0.5, (p012.y + p123.y) * 0.5);

        flattenCubic(p0, p01, p012, p0123, out, tol, depth + 1);
        flattenCubic(p0123, p123, p23, p3, out, tol, depth + 1);
    }

    static int orient(const Vec& a, const Vec& b, const Vec& c) {
        const double v = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        return (v > EPS) - (v < -EPS);
    }

    static bool onSegment(const Vec& a, const Vec& b, const Vec& p) {
        if (orient(a, b, p) != 0) return false;
        return std::min(a.x, b.x) - EPS <= p.x && p.x <= std::max(a.x, b.x) + EPS &&
            std::min(a.y, b.y) - EPS <= p.y && p.y <= std::max(a.y, b.y) + EPS;
    }

    static bool segsIntersectProper(const Vec& a1, const Vec& a2, const Vec& b1, const Vec& b2) {
        const int o1 = orient(a1, a2, b1);
        const int o2 = orient(a1, a2, b2);
        const int o3 = orient(b1, b2, a1);
        const int o4 = orient(b1, b2, a2);

        if (o1 != o2 && o3 != o4) return true;

        if (o1 == 0 && onSegment(a1, a2, b1)) return true;
        if (o2 == 0 && onSegment(a1, a2, b2)) return true;
        if (o3 == 0 && onSegment(b1, b2, a1)) return true;
        if (o4 == 0 && onSegment(b1, b2, a2)) return true;

        return false;
    }

    static bool hasSelfIntersections(const std::vector<Vec>& pts) {
        const size_t n = pts.size();
        if (n < 4) return false;

        for (size_t i = 0; i < n; ++i) {
            const Vec& a1 = pts[i];
            const Vec& a2 = pts[(i + 1) % n];

            if (Vec(a2.x - a1.x, a2.y - a1.y).lengthSq() <= EPS * EPS) continue;

            for (size_t j = i + 2; j < n; ++j) {
                if (i == 0 && j == n - 1) continue;

                const Vec& b1 = pts[j];
                const Vec& b2 = pts[(j + 1) % n];

                if (Vec(b2.x - b1.x, b2.y - b1.y).lengthSq() <= EPS * EPS) continue;

                if (segsIntersectProper(a1, a2, b1, b2)) {
                    const bool shareEndpoint =
                        (Vec(a1.x - b1.x, a1.y - b1.y).lengthSq() <= EPS * EPS) ||
                        (Vec(a1.x - b2.x, a1.y - b2.y).lengthSq() <= EPS * EPS) ||
                        (Vec(a2.x - b1.x, a2.y - b1.y).lengthSq() <= EPS * EPS) ||
                        (Vec(a2.x - b2.x, a2.y - b2.y).lengthSq() <= EPS * EPS);
                    if (!shareEndpoint) return true;
                }
            }
        }
        return false;
    }

    struct DecomposeCtx {
        std::vector<std::vector<Vec>> contours;
        std::vector<Vec> current;
        Vec lastMove;
        double tol;
        DecomposeCtx() : tol(FLATTEN_EPS) {}
    };

    static int move_to_func_internal(const FT_Vector* to, void* user) {
        DecomposeCtx* ctx = static_cast<DecomposeCtx*>(user);
        if (!ctx->current.empty()) {
            ctx->contours.push_back(std::move(ctx->current));
            ctx->current.clear();
        }
        ctx->lastMove = Vec(static_cast<double>(to->x), static_cast<double>(to->y));
        ctx->current.push_back(ctx->lastMove);
        return 0;
    }

    static int line_to_func_internal(const FT_Vector* to, void* user) {
        DecomposeCtx* ctx = static_cast<DecomposeCtx*>(user);
        ctx->current.emplace_back(static_cast<double>(to->x), static_cast<double>(to->y));
        return 0;
    }

    static int conic_to_func_internal(const FT_Vector* control, const FT_Vector* to, void* user) {
        DecomposeCtx* ctx = static_cast<DecomposeCtx*>(user);
        if (ctx->current.empty()) return -1;

        const Vec& p0 = ctx->current.back();
        const Vec p1(static_cast<double>(control->x), static_cast<double>(control->y));
        const Vec p2(static_cast<double>(to->x), static_cast<double>(to->y));

        flattenQuadratic(p0, p1, p2, ctx->current, ctx->tol);
        return 0;
    }

    static int cubic_to_func_internal(const FT_Vector* c1, const FT_Vector* c2, const FT_Vector* to, void* user) {
        DecomposeCtx* ctx = static_cast<DecomposeCtx*>(user);
        if (ctx->current.empty()) return -1;

        const Vec& p0 = ctx->current.back();
        const Vec p1(static_cast<double>(c1->x), static_cast<double>(c1->y));
        const Vec p2(static_cast<double>(c2->x), static_cast<double>(c2->y));
        const Vec p3(static_cast<double>(to->x), static_cast<double>(to->y));

        flattenCubic(p0, p1, p2, p3, ctx->current, ctx->tol);
        return 0;
    }

    static bool validateOutline(const FT_Outline& outline, double tol) {
        DecomposeCtx ctx;
        ctx.tol = tol;

        FT_Outline_Funcs funcs = {};
        funcs.move_to = move_to_func_internal;
        funcs.line_to = line_to_func_internal;
        funcs.conic_to = conic_to_func_internal;
        funcs.cubic_to = cubic_to_func_internal;
        funcs.shift = 0;
        funcs.delta = 0;

        if (FT_Outline_Decompose(const_cast<FT_Outline*>(&outline), &funcs, &ctx) != 0) {
            return false;
        }
        if (!ctx.current.empty()) {
            ctx.contours.push_back(std::move(ctx.current));
        }
        for (const auto& cont : ctx.contours) {
            if (cont.size() < MIN_CONTOUR_SIZE) return false;
            for (const auto& p : cont) {
                if (!isValidCoord(p.x) || !isValidCoord(p.y)) return false;
            }
            bool hasNonDegenerateEdge = false;
            for (size_t i = 0; i < cont.size(); ++i) {
                const Vec& a = cont[i];
                const Vec& b = cont[(i + 1) % cont.size()];
                if (Vec(b.x - a.x, b.y - a.y).lengthSq() > EPS * EPS) {
                    hasNonDegenerateEdge = true;
                    break;
                }
            }
            if (!hasNonDegenerateEdge) return false;
            if (hasSelfIntersections(cont)) return false;
        }
        return true;
    }

    static std::vector<Vec> flattenMsdfContour(const msdfgen::Contour& contour, double tol) {
        std::vector<Vec> result;

        for (size_t i = 0; i < contour.edges.size(); ++i) {
            const msdfgen::EdgeSegment* edge = contour.edges[i];
            if (!edge) continue;

            const msdfgen::Point2 start = edge->point(0);

            if (result.empty()) {
                result.emplace_back(start.x, start.y);
            }

            std::vector<Vec> edgePoints;
            edgePoints.emplace_back(start.x, start.y);

            for (int j = 1; j <= MAX_CURVE_SAMPLES; ++j) {
                const double t = static_cast<double>(j) / MAX_CURVE_SAMPLES;
                const msdfgen::Point2 pt = edge->point(t);
                edgePoints.emplace_back(pt.x, pt.y);
            }

            bool isLinear = true;
            const Vec v0 = edgePoints.front();
            const Vec vEnd = edgePoints.back();

            for (size_t j = 1; j < edgePoints.size() - 1; ++j) {
                if (distPointToLine(edgePoints[j], v0, vEnd) > tol) {
                    isLinear = false;
                    break;
                }
            }

            if (isLinear) {
                result.push_back(edgePoints.back());
            }
            else {
                for (size_t j = 1; j < edgePoints.size(); ++j) {
                    result.push_back(edgePoints[j]);
                }
            }
        }
        return result;
    }

    static bool validateResolvedShape(const msdfgen::Shape& shape, double tol) {
        if (shape.contours.empty()) return false;

        for (const msdfgen::Contour& contour : shape.contours) {
            if (contour.edges.empty()) return false;

            std::vector<Vec> pts = flattenMsdfContour(contour, tol);
            if (pts.size() < MIN_CONTOUR_SIZE) return false;

            for (const auto& p : pts) {
                if (!isValidCoord(p.x) || !isValidCoord(p.y)) {
                    return false;
                }
            }

            bool hasNonDegenerateEdge = false;
            for (size_t i = 0; i < pts.size(); ++i) {
                const Vec& a = pts[i];
                const Vec& b = pts[(i + 1) % pts.size()];
                if (Vec(b.x - a.x, b.y - a.y).lengthSq() > EPS * EPS) {
                    hasNonDegenerateEdge = true;
                    break;
                }
            }
            if (!hasNonDegenerateEdge) return false;
            if (hasSelfIntersections(pts)) return false;
        }
        return true;
    }

    static bool IsGlyphValid(msdfgen::FontHandle* font, uint32_t codepoint, double tol = FLATTEN_EPS) {
        if (!font) {
            return false;
        }
        msdfgen::Shape shape;
        if (!msdfgen::loadGlyph(shape, font, codepoint))  return false;
        if (shape.contours.empty()) return true;
        msdfgen::resolveShapeGeometry(shape);
        return validateResolvedShape(shape, tol);
    }
};
