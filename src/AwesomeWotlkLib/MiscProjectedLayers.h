#pragma once

#include <xmmintrin.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>

#include "include/Graphics/ProjectedTexture.h"
#include "include/Math/Primitives.h"

namespace misc_projected_layers {
struct RampPlane {
    Vec3f pivot;
    float slope_x;
    float slope_y;
    float half_height;

    [[nodiscard]]
    float heightAt(float x, float y) const {
        return pivot.z + slope_x * (x - pivot.x) + slope_y * (y - pivot.y);
    }
};

inline Mat4f rampMatrix(const RampPlane& plane, const Vec3f& camera, float z_bias) {
    const float scale = 0.5f / plane.half_height;
    const float from_x = -plane.slope_x * scale;
    const float from_y = -plane.slope_y * scale;
    const Vec3f offset = camera - plane.pivot;
    const float constant = offset.x * from_x + offset.y * from_y + offset.z * scale + z_bias;

    Mat4f ramp;
    ramp.m[0][0] = ramp.m[0][2] = from_x;
    ramp.m[1][0] = ramp.m[1][2] = from_y;
    ramp.m[2][0] = ramp.m[2][2] = scale;
    ramp.m[3][0] = ramp.m[3][2] = constant;
    ramp.m[3][1] = 0.5f;
    ramp.m[3][3] = 1.0f;
    return ramp;
}

[[nodiscard]]
inline float bandHalf(float radius, float own_half) {
    static constexpr float kPerRadius = 1.7876304f;
    static constexpr float kMinHalf = 3.0f;
    static constexpr float kMaxHalf = 50.0f;
    return std::max(own_half, std::clamp(radius * kPerRadius, kMinHalf, kMaxHalf));
}

class ProjectedLayers {
public:
    static constexpr int kCells = 32;
    static constexpr int kColumns = kCells + 1;

    enum class Outcome {
        eAllTerrain,
        eNotStacked,
        eNoSurface,
        eNarrow,
        eHeld,
        eWallStart,
    };

    struct Report {
        Outcome outcome;
        uint32_t triangles;
        int stacked;
        int reached;
    };

    struct ReachMask {
        float origin_x;
        float origin_y;
        float cell;
        std::array<uint8_t, kColumns * kColumns> covered;
    };

    void arm(const AaBox& box, float z_bias, bool eligible) {
        radius_ = (box.max.x - box.min.x) * 0.5f;
        wide_half_ = (box.max.z - box.min.z) * 0.5f;
        armed_ = eligible && radius_ > 0.0f;
        z_bias_ = z_bias;
        hit_ = (box.min + box.max) * 0.5f;
        if (armed_) {
            cell_ = 2.0f * radius_ / kCells;
            origin_x_ = std::floor(box.min.x / cell_) * cell_ - hit_.x;
            origin_y_ = std::floor(box.min.y / cell_) * cell_ - hit_.y;
            for (int i = 0; i < kCenters; ++i) {
                const float offset = (static_cast<float>(i) + 0.5f) * cell_;
                center_x_[i] = origin_x_ + offset;
                center_y_[i] = origin_y_ + offset;
            }
        }
    }

    [[nodiscard]]
    float wideHalf() const {
        return wide_half_;
    }

    [[nodiscard]]
    float zBias() const {
        return z_bias_;
    }

    [[nodiscard]]
    const Report& report() const {
        return report_;
    }

    [[nodiscard]]
    const ReachMask& reachMask() const {
        return reach_;
    }

    [[nodiscard]]
    std::optional<RampPlane> solve(std::span<const ProjectedBatch> batches) {
        if (!std::exchange(armed_, false)) { return std::nullopt; }

        const std::optional<RampPlane> held = std::exchange(held_, std::nullopt);

        uint32_t triangles = 0;
        bool stackable = false;
        for (const ProjectedBatch& batch : batches) {
            triangles += batch.index_count / 3u;
            stackable = stackable || !batch.terrain();
        }
        report_ = {.outcome = Outcome::eAllTerrain, .triangles = triangles, .stacked = 0, .reached = 0};
        if (!stackable) { return std::nullopt; }

        std::ranges::fill(counts_, uint8_t{0});
        hit_on_wall_ = false;
        for (const ProjectedBatch& batch : batches) {
            if (!batch.drawable()) { continue; }
            const std::span indices(batch.indices, batch.index_count);
            const projected_texture::BatchPoints points(batch);
            for (uint32_t i = 0; i + 2 < batch.index_count; i += 3) {
                add(points[indices[i]] - hit_, points[indices[i + 1]] - hit_, points[indices[i + 2]] - hit_);
            }
        }
        report_.stacked = stackedColumns();
        if (report_.stacked < (held ? kKeepStacked : kMinStacked)) {
            report_.outcome = Outcome::eNotStacked;
            return std::nullopt;
        }

        report_.reached = flood();
        if (report_.reached < (held ? kKeepReached : kMinReached)) {
            const bool in_reach = held && solvedAtThisRadius() &&
                std::hypot(hit_.x - held_at_.x, hit_.y - held_at_.y) < radius_ &&
                std::abs(held->heightAt(hit_.x, hit_.y) - hit_.z) < wide_half_;
            if (in_reach) {
                report_.outcome = Outcome::eHeld;
                held_ = held;
                followReach();
                return held_;
            }

            const int started = floodNear();
            if (started < kMinReached) {
                report_.outcome = Outcome::eNoSurface;
                return std::nullopt;
            }
            report_.outcome = Outcome::eWallStart;
            report_.reached = started;
        } else {
            report_.outcome = Outcome::eNarrow;
        }
        held_ = fit(report_.reached);
        held_at_ = hit_;
        keepSolve(report_.reached);
        keepReach(report_.reached);
        return held_;
    }

private:
    static constexpr int kCenters = (kColumns + 3) / 4 * 4;
    static constexpr int kLayers = 6;
    static constexpr float kLayerMerge = 0.2f;
    static constexpr float kWallCos = 0.17364818f;

    static constexpr float kStackGap = 2.0f;
    static constexpr int kMinStacked = 4;
    static constexpr int kKeepStacked = 2;
    static constexpr int kMinReached = 40;
    static constexpr int kKeepReached = 26;

    static constexpr float kSeedReach = 1.5f;
    static constexpr float kSeedBias = 0.02f;
    static constexpr float kOnWall = 0.05f;
    static constexpr float kMaxClimb = 2.1445069f;

    static constexpr double kRidge = 1e-3;
    static constexpr float kResidualShare = 0.95f;
    static constexpr float kResidualMargin = 0.25f;

    static constexpr float kSameRadius = 0.01f;

    struct Surface {
        float z;
        float slope_x;
        float slope_y;
        float grade;
    };

    struct SolvedColumn {
        int column;
        Surface surface;
    };

    [[nodiscard]]
    float centerX(int cell) const {
        return center_x_[cell];
    }

    [[nodiscard]]
    float centerY(int cell) const {
        return center_y_[cell];
    }

    [[nodiscard]]
    bool inCircle(int x, int y) const {
        const float px = centerX(x);
        const float py = centerY(y);
        return px * px + py * py <= radius_ * radius_;
    }

    [[nodiscard]]
    bool solvedAtThisRadius() const {
        return std::abs(solved_cell_ - cell_) < kSameRadius * cell_;
    }

    [[nodiscard]]
    static bool holdsHit(const Vec3f& a, const Vec3f& b, const Vec3f& c, const Vec3f& n, float length) {
        if (length <= 0.0f || std::abs(a.dot(n)) > kOnWall * length) { return false; }
        const float margin = -kOnWall * length;
        return a.cross(b).dot(n) >= margin && b.cross(c).dot(n) >= margin && c.cross(a).dot(n) >= margin;
    }

    void add(Vec3f a, Vec3f b, Vec3f c) {
        const Vec3f n = (b - a).cross(c - a);
        const float area = std::abs(n.z);
        const float length = std::sqrt(n.lengthSq());
        if (area <= 1e-4f || area < kWallCos * length) {
            hit_on_wall_ = hit_on_wall_ || holdsHit(a, b, c, n, length);
            return;
        }
        if (n.z < 0.0f) { std::swap(b, c); }

        const __m128 xy_a = _mm_setr_ps(a.x, a.y, 0.0f, 0.0f);
        const __m128 xy_b = _mm_setr_ps(b.x, b.y, 0.0f, 0.0f);
        const __m128 xy_c = _mm_setr_ps(c.x, c.y, 0.0f, 0.0f);
        const __m128 corners =
            _mm_movelh_ps(_mm_min_ps(_mm_min_ps(xy_a, xy_b), xy_c), _mm_max_ps(_mm_max_ps(xy_a, xy_b), xy_c));
        const __m128 origin = _mm_setr_ps(origin_x_, origin_y_, origin_x_, origin_y_);
        const __m128 extent = _mm_div_ps(_mm_sub_ps(corners, origin), _mm_set1_ps(cell_));
        static constexpr float kEnd = kColumns;
        if (_mm_movemask_ps(_mm_cmplt_ps(extent, _mm_setr_ps(kEnd, kEnd, 0.0f, 0.0f))) != 0b0011) { return; }

        const float slope_x = -n.x / n.z;
        const float slope_y = -n.y / n.z;
        const float grade = std::sqrt(slope_x * slope_x + slope_y * slope_y);
        std::array<float, 4> cells{};
        _mm_storeu_ps(cells.data(), extent);
        const int x0 = std::max(0, static_cast<int>(cells[0]));
        const int y0 = std::max(0, static_cast<int>(cells[1]));
        const int x1 = std::min(kColumns - 1, static_cast<int>(cells[2]));
        const int y1 = std::min(kColumns - 1, static_cast<int>(cells[3]));

        const __m128 a_x = _mm_set1_ps(a.x);
        const __m128 b_x = _mm_set1_ps(b.x);
        const __m128 c_x = _mm_set1_ps(c.x);
        const __m128 a_z = _mm_set1_ps(a.z);
        const __m128 b_z = _mm_set1_ps(b.z);
        const __m128 c_z = _mm_set1_ps(c.z);
        const __m128 edge0 = _mm_set1_ps(c.y - b.y);
        const __m128 edge1 = _mm_set1_ps(a.y - c.y);
        const __m128 edge2 = _mm_set1_ps(b.y - a.y);
        const __m128 divisor = _mm_set1_ps(area);
        const __m128 zero = _mm_setzero_ps();
        for (int y = y0; y <= y1; ++y) {
            const float py = centerY(y);
            const __m128 row0 = _mm_set1_ps((c.x - b.x) * (py - b.y));
            const __m128 row1 = _mm_set1_ps((a.x - c.x) * (py - c.y));
            const __m128 row2 = _mm_set1_ps((b.x - a.x) * (py - a.y));
            for (int x = x0; x <= x1; x += 4) {
                const __m128 px = _mm_loadu_ps(&center_x_[x]);
                const __m128 w0 = _mm_sub_ps(row0, _mm_mul_ps(edge0, _mm_sub_ps(px, b_x)));
                const __m128 w1 = _mm_sub_ps(row1, _mm_mul_ps(edge1, _mm_sub_ps(px, c_x)));
                const __m128 w2 = _mm_sub_ps(row2, _mm_mul_ps(edge2, _mm_sub_ps(px, a_x)));
                const __m128 outside =
                    _mm_or_ps(_mm_or_ps(_mm_cmplt_ps(w0, zero), _mm_cmplt_ps(w1, zero)), _mm_cmplt_ps(w2, zero));
                const int count = x1 - x + 1;
                const int columns = count < 4 ? (1 << count) - 1 : 0b1111;
                const auto inside = static_cast<unsigned>(~_mm_movemask_ps(outside) & columns);
                if (inside == 0) { continue; }

                const __m128 z = _mm_add_ps(_mm_add_ps(_mm_mul_ps(w0, a_z), _mm_mul_ps(w1, b_z)), _mm_mul_ps(w2, c_z));
                std::array<float, 4> heights{};
                _mm_storeu_ps(heights.data(), _mm_div_ps(z, divisor));
                for (unsigned left = inside; left != 0; left &= left - 1) {
                    const int lane = std::countr_zero(left);
                    insert(y * kColumns + x + lane,
                        {.z = heights[lane], .slope_x = slope_x, .slope_y = slope_y, .grade = grade});
                }
            }
        }
    }

    void insert(int column, const Surface& layer) {
        const auto layers = std::span(layers_[column]).first(counts_[column]);
        if (std::ranges::any_of(
                layers, [&](const Surface& other) { return std::abs(other.z - layer.z) < kLayerMerge; })) {
            return;
        }
        if (counts_[column] < kLayers) {
            layers_[column][counts_[column]++] = layer;
            return;
        }
        Surface& farthest =
            *std::ranges::max_element(layers, {}, [](const Surface& other) { return std::abs(other.z); });
        if (std::abs(layer.z) < std::abs(farthest.z)) { farthest = layer; }
    }

    [[nodiscard]]
    int stackedColumns() const {
        int stacked = 0;
        for (int y = 0; y < kColumns; ++y) {
            for (int x = 0; x < kColumns; ++x) {
                const int column = y * kColumns + x;
                if (counts_[column] < 2 || !inCircle(x, y)) { continue; }
                const auto layers = std::span(layers_[column]).first(counts_[column]);
                const auto [lowest, highest] = std::ranges::minmax_element(layers, {}, &Surface::z);
                stacked += highest->z - lowest->z > kStackGap ? 1 : 0;
            }
        }
        return stacked;
    }

    [[nodiscard]]
    int flood() {
        int seed = -1;
        Surface seed_surface{};
        float nearest = hit_on_wall_ ? kOnWall : kSeedReach;
        const int hit_x = static_cast<int>(-origin_x_ / cell_);
        const int hit_y = static_cast<int>(-origin_y_ / cell_);
        for (int y = hit_y - 1; y <= hit_y + 1; ++y) {
            for (int x = hit_x - 1; x <= hit_x + 1; ++x) {
                const int column = y * kColumns + x;
                for (const Surface& layer : std::span(layers_[column]).first(counts_[column])) {
                    const float gap = std::abs(layer.z - layer.slope_x * centerX(x) - layer.slope_y * centerY(y)) +
                        kSeedBias * static_cast<float>(std::abs(x - hit_x) + std::abs(y - hit_y));
                    if (gap < nearest) {
                        nearest = gap;
                        seed = column;
                        seed_surface = layer;
                    }
                }
            }
        }
        return seed < 0 ? 0 : spread(seed, seed_surface);
    }

    [[nodiscard]]
    int floodNear() {
        static constexpr int kAround = 3;
        static constexpr int kMaxTries = 4;
        static constexpr float kSameLayer = 0.5f;

        struct Start {
            float gap;
            int column;
            Surface surface;
        };

        std::array<Start, (2 * kAround + 1) * (2 * kAround + 1) * kLayers> starts{};
        size_t count = 0;
        const int hit_x = static_cast<int>(-origin_x_ / cell_);
        const int hit_y = static_cast<int>(-origin_y_ / cell_);
        for (int y = std::max(0, hit_y - kAround); y <= std::min(kColumns - 1, hit_y + kAround); ++y) {
            for (int x = std::max(0, hit_x - kAround); x <= std::min(kColumns - 1, hit_x + kAround); ++x) {
                const int column = y * kColumns + x;
                for (const Surface& layer : std::span(layers_[column]).first(counts_[column])) {
                    if (layer.grade > kMaxClimb) { continue; }
                    const float gap =
                        std::abs(layer.z) + kSeedBias * static_cast<float>(std::abs(x - hit_x) + std::abs(y - hit_y));
                    if (gap < wide_half_) { starts[count++] = {.gap = gap, .column = column, .surface = layer}; }
                }
            }
        }
        std::sort(starts.begin(), starts.begin() + static_cast<std::ptrdiff_t>(count),
            [](const Start& a, const Start& b) { return a.gap < b.gap; });

        std::array<float, kMaxTries> tried{};
        size_t tries = 0;
        for (const Start& start : std::span(starts).first(count)) {
            const float z = start.surface.z;
            const auto same_layer = [z](float other) { return std::abs(other - z) < kSameLayer; };
            if (std::ranges::any_of(std::span(tried).first(tries), same_layer)) { continue; }

            tried[tries++] = z;
            const int reached = spread(start.column, start.surface);
            if (reached >= kMinReached) { return reached; }
            if (tries == kMaxTries) { break; }
        }
        return 0;
    }

    [[nodiscard]]
    int spread(int seed, const Surface& seed_surface) {
        std::ranges::fill(reached_, false);
        reached_[seed] = true;
        surface_[seed] = seed_surface;
        queue_[0] = static_cast<uint16_t>(seed);
        return walk(1);
    }

    [[nodiscard]]
    int walk(int tail) {
        static constexpr std::array<std::array<int, 2>, 4> kSteps = {{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}};
        const float max_step = cell_ * kMaxClimb;
        for (int head = 0; head < tail; ++head) {
            const int column = queue_[head];
            const Surface from = surface_[column];
            for (const auto& [dx, dy] : kSteps) {
                const int x = column % kColumns + dx;
                const int y = column / kColumns + dy;
                if (x < 0 || y < 0 || x >= kColumns || y >= kColumns || !inCircle(x, y)) { continue; }
                const int next = y * kColumns + x;
                if (reached_[next]) { continue; }

                const float predicted =
                    from.z + (static_cast<float>(dx) * from.slope_x + static_cast<float>(dy) * from.slope_y) * cell_;
                float best_gap = std::numeric_limits<float>::infinity();
                const Surface* picked = nullptr;
                for (const Surface& layer : std::span(layers_[next]).first(counts_[next])) {
                    if (layer.grade > kMaxClimb || std::abs(layer.z - from.z) > max_step) { continue; }
                    const float gap = std::abs(layer.z - predicted);
                    if (gap < best_gap) {
                        best_gap = gap;
                        picked = &layer;
                    }
                }
                if (picked == nullptr) { continue; }

                surface_[next] = *picked;
                reached_[next] = true;
                queue_[tail++] = static_cast<uint16_t>(next);
            }
        }
        return tail;
    }

    void keepSolve(int reached) {
        solved_x_ = hit_.x + origin_x_;
        solved_y_ = hit_.y + origin_y_;
        solved_z_ = hit_.z;
        solved_cell_ = cell_;
        solved_count_ = reached;
        for (int i = 0; i < reached; ++i) { solved_[i] = {.column = queue_[i], .surface = surface_[queue_[i]]}; }
    }

    void followReach() {
        const int shift_x = static_cast<int>(std::lround((solved_x_ - hit_.x - origin_x_) / cell_));
        const int shift_y = static_cast<int>(std::lround((solved_y_ - hit_.y - origin_y_) / cell_));
        std::ranges::fill(reached_, false);
        int tail = 0;
        for (const auto& [column, surface] : std::span(solved_).first(static_cast<size_t>(solved_count_))) {
            const int x = column % kColumns + shift_x;
            const int y = column / kColumns + shift_y;
            if (x < 0 || y < 0 || x >= kColumns || y >= kColumns || !inCircle(x, y)) { continue; }

            const int col = y * kColumns + x;
            reached_[col] = true;
            surface_[col] = surface;
            surface_[col].z += solved_z_ - hit_.z;
            queue_[tail++] = static_cast<uint16_t>(col);
        }
        keepReach(walk(tail));
    }

    void keepReach(int reached) {
        reach_.origin_x = hit_.x + origin_x_;
        reach_.origin_y = hit_.y + origin_y_;
        reach_.cell = cell_;
        reach_.covered.fill(0);
        for (const int column : std::span(queue_).first(static_cast<size_t>(reached))) {
            const int x = column % kColumns;
            const int y = column / kColumns;
            for (int around_y = std::max(0, y - 1); around_y <= std::min(kColumns - 1, y + 1); ++around_y) {
                for (int around_x = std::max(0, x - 1); around_x <= std::min(kColumns - 1, x + 1); ++around_x) {
                    reach_.covered[around_y * kColumns + around_x] = 1;
                }
            }
        }
    }

    [[nodiscard]]
    RampPlane fit(int reached) {
        const auto columns = std::span(queue_).first(static_cast<size_t>(reached));
        double mean_x = 0.0;
        double mean_y = 0.0;
        double mean_z = 0.0;
        for (const int column : columns) {
            mean_x += centerX(column % kColumns);
            mean_y += centerY(column / kColumns);
            mean_z += surface_[column].z;
        }
        const double count = reached;
        mean_x /= count;
        mean_y /= count;
        mean_z /= count;

        double xx = kRidge * count * radius_ * radius_;
        double yy = xx;
        double xy = 0.0;
        double xz = 0.0;
        double yz = 0.0;
        for (const int column : columns) {
            const double dx = centerX(column % kColumns) - mean_x;
            const double dy = centerY(column / kColumns) - mean_y;
            const double dz = surface_[column].z - mean_z;
            xx += dx * dx;
            yy += dy * dy;
            xy += dx * dy;
            xz += dx * dz;
            yz += dy * dz;
        }
        const double determinant = xx * yy - xy * xy;
        double slope_x = (xz * yy - yz * xy) / determinant;
        double slope_y = (yz * xx - xz * xy) / determinant;
        const double grade = std::sqrt(slope_x * slope_x + slope_y * slope_y);
        if (grade > kMaxClimb) {
            slope_x *= kMaxClimb / grade;
            slope_y *= kMaxClimb / grade;
        }

        for (int i = 0; i < reached; ++i) {
            const int column = queue_[i];
            const double plane = mean_z + slope_x * (centerX(column % kColumns) - mean_x) +
                slope_y * (centerY(column / kColumns) - mean_y);
            off_plane_[i] = static_cast<float>(std::abs(surface_[column].z - plane));
        }
        const auto off_plane = std::span(off_plane_).first(static_cast<size_t>(reached));
        const auto residual =
            off_plane.begin() + static_cast<std::ptrdiff_t>(static_cast<float>(reached - 1) * kResidualShare);
        std::ranges::nth_element(off_plane, residual);
        const float half_height = projected_texture::kRampPerFullAlpha * (*residual + kResidualMargin);
        return {
            .pivot = {hit_.x + static_cast<float>(mean_x), hit_.y + static_cast<float>(mean_y),
                hit_.z + static_cast<float>(mean_z)},
            .slope_x = static_cast<float>(slope_x),
            .slope_y = static_cast<float>(slope_y),
            .half_height = std::clamp(half_height, projected_texture::kReticleHalf, wide_half_)
        };
    }

    bool armed_{};
    bool hit_on_wall_{};
    Report report_{};
    Vec3f hit_;
    float radius_{};
    float wide_half_{};
    float cell_{};
    float origin_x_{};
    float origin_y_{};
    float z_bias_{};
    std::optional<RampPlane> held_;
    Vec3f held_at_;
    float solved_x_{};
    float solved_y_{};
    float solved_z_{};
    float solved_cell_{};
    int solved_count_{};
    std::array<SolvedColumn, kColumns * kColumns> solved_{};
    ReachMask reach_{};
    std::array<float, kCenters> center_x_{};
    std::array<float, kCenters> center_y_{};
    std::array<std::array<Surface, kLayers>, kColumns * kColumns> layers_{};
    std::array<uint8_t, kColumns * kColumns> counts_{};
    std::array<Surface, kColumns * kColumns> surface_{};
    std::array<bool, kColumns * kColumns> reached_{};
    std::array<uint16_t, kColumns * kColumns> queue_{};
    std::array<float, kColumns * kColumns> off_plane_{};
};

inline ProjectedLayers layers;

inline constexpr int kReachTexels = 64;
inline constexpr int kReachMargin = 1;
inline constexpr uint32_t kReachCovered = 0xFFFFFFFFu;
inline constexpr uint32_t kReachEmpty = 0x00FFFFFFu;

inline Mat4f reachMatrix(const ProjectedLayers::ReachMask& mask, const Vec3f& camera) {
    const float scale = 1.0f / (mask.cell * kReachTexels);
    const float margin = mask.cell * kReachMargin;
    Mat4f matrix;
    matrix.m[0][0] = scale;
    matrix.m[1][1] = scale;
    matrix.m[2][2] = 1.0f;
    matrix.m[3][0] = (camera.x - mask.origin_x + margin) * scale;
    matrix.m[3][1] = (camera.y - mask.origin_y + margin) * scale;
    matrix.m[3][3] = 1.0f;
    return matrix;
}

inline void writeReachTexels(
    const ProjectedLayers::ReachMask& mask, std::span<uint32_t, kReachTexels * kReachTexels> texels) {
    for (int y = 0; y < ProjectedLayers::kColumns; ++y) {
        for (int x = 0; x < ProjectedLayers::kColumns; ++x) {
            texels[(y + kReachMargin) * kReachTexels + x + kReachMargin] =
                mask.covered[y * ProjectedLayers::kColumns + x] != 0 ? kReachCovered : kReachEmpty;
        }
    }
}
}  // namespace misc_projected_layers
