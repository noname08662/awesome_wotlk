#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

#include "include/Math/Primitives.h"

namespace misc_ground_target_clamp {
struct GroundHit {
    Vec3f pos;
    uint64_t object = 0;
};

struct Query {
    Vec3f caster;
    Vec3f ray_start;
    Vec3f camera_forward;
    float min_range = 0.0f;
    float max_range = 0.0f;
    std::optional<Vec3f> shown;
};

inline constexpr float kClampReach = 30.0f;
inline constexpr float kRangeMargin = 0.001f;
inline constexpr float kMinFacing = 1.1920929e-7f;
inline constexpr float kDropHeight = 50.0f;
inline constexpr float kRefineStep = 0.05f;
inline constexpr float kCloseEnough = 0.1f;
inline constexpr int kMaxTraces = 15;

inline constexpr float kSlopeStep = 0.1f;
inline constexpr float kPickPast = 10.0f;
inline constexpr float kFloorNormal = 0.5f;
inline constexpr float kCeilingNormal = -0.5f;
inline constexpr float kMarchFactor = 2.0f;
inline constexpr float kSameRay = 2e-4f;
inline constexpr float kMaxStep = 5.0f;
inline constexpr float kLedge = 2.0f;
inline constexpr float kReadReach = 10.0f;
inline constexpr float kFall = 10.0f;
inline constexpr float kSeenLift = 1.0f;

inline constexpr float kMinRangeDeadzoneSq = 3.0625f;
inline constexpr float kMinRangeLift = 3.0f;
inline constexpr float kBlockedSq = 0.05f;

inline bool overDisc(
    const Vec3f& from, const Vec3f& dir, const Vec3f& center, float radius, float& enter, float& leave) {
    const float x = from.x - center.x;
    const float y = from.y - center.y;
    const float a = dir.x * dir.x + dir.y * dir.y;
    const float b = x * dir.x + y * dir.y;
    const float c = x * x + y * y - radius * radius;
    const float discriminant = b * b - a * c;
    if (a < kMinFacing || discriminant < 0.0f) { return false; }

    const float root = std::sqrt(discriminant);
    enter = (-b - root) / a;
    leave = (-b + root) / a;
    return true;
}

template <typename World>
bool inView(const Query& q, const Vec3f& point) {
    Vec3f blocker;
    uint64_t unused = 0;
    return !World::surface(q.ray_start, Vec3f{point.x, point.y, point.z + kSeenLift}, blocker, unused);
}

template <typename World>
bool walkToMaxRange(
    const Query& q, const Vec3f& aim, float min_range, float max_range, GroundHit& out, bool seen_only = false) {
    const Vec3f ray = aim - q.ray_start;
    const float facing = ray.dot(q.camera_forward);
    if (std::abs(facing) < kMinFacing) { return false; }

    const float t = -(q.ray_start - q.caster).dot(q.camera_forward) / facing;
    if (t < 0.0f || t > 1.0f) { return false; }

    const Vec3f origin = q.ray_start + ray * t;
    const float span = static_cast<float>((aim - origin).length());
    if (span < kRangeMargin) { return false; }
    const Vec3f dir = (aim - origin) / span;

    float enter = 0.0f;
    float leave = 0.0f;
    if (!overDisc(origin, dir, q.caster, max_range, enter, leave)) { return false; }
    float near_end = std::max(enter, 0.0f);
    float far_end = std::min(leave, std::min(span, max_range) - kRangeMargin);
    if (far_end <= near_end) { return false; }

    const float target = max_range - kCloseEnough * 0.5f;
    const auto ringAlong = [&](float rise, float& along) {
        const float radius_sq = target * target - rise * rise;
        float unused = 0.0f;
        return radius_sq > 0.0f && overDisc(origin, dir, q.caster, std::sqrt(radius_sq), unused, along) &&
            along > near_end && along < far_end;
    };

    float along = 0.0f;
    if (!ringAlong(0.0f, along)) { along = far_end; }

    float best = 0.0f;
    bool found = false;
    for (int i = 0; i < kMaxTraces; ++i) {
        const Vec3f sample = origin + dir * along;
        Vec3f ground;
        uint64_t object = 0;
        const bool hit = World::pick(sample, Vec3f{sample.x, sample.y, sample.z - kDropHeight}, ground, object);
        const float dist = hit ? static_cast<float>((ground - q.caster).length()) : std::numeric_limits<float>::max();
        const bool hidden = seen_only && dist <= max_range && !inView<World>(q, ground);
        if (dist > max_range || hidden) {
            far_end = along;
        } else {
            near_end = along;
            if (dist > best && dist >= min_range) {
                out = {.pos = ground, .object = object};
                best = dist;
                found = true;
                if (max_range - dist < kCloseEnough) { return true; }
            }
        }

        if (far_end - near_end < kRefineStep) { break; }
        if (!hit || hidden || !ringAlong(ground.z - q.caster.z, along)) { along = (near_end + far_end) * 0.5f; }
    }
    return found;
}

template <typename World>
bool dropToMaxRange(const Query& q, const Vec3f& aim, float min_range, float max_range, GroundHit& out) {
    if (!walkToMaxRange<World>(q, aim, min_range, max_range, out)) { return false; }
    const float height = q.shown ? q.shown->z : q.caster.z;
    if (height - out.pos.z <= kFall || inView<World>(q, out.pos)) { return true; }

    GroundHit ledge;
    if (walkToMaxRange<World>(q, aim, min_range, max_range, ledge, true) && ledge.pos.z - out.pos.z > kFall) {
        out = ledge;
    }
    return true;
}

inline float normalZ(const Vec3f& eye, const Vec3f& from, const Vec3f& to) {
    const Vec3f tangent = to - from;
    const float tangent_sq = tangent.lengthSq();
    if (tangent_sq < kMinFacing) { return 1.0f; }

    const Vec3f to_eye = eye - from;
    const Vec3f facing = to_eye - tangent * (to_eye.dot(tangent) / tangent_sq);
    const auto length = static_cast<float>(facing.length());
    return length > 0.0f ? facing.z / length : 1.0f;
}

inline Vec3f chordCrossing(const Vec3f& outside, const Vec3f& inside, const Vec3f& center, float radius) {
    const Vec3f chord = inside - outside;
    const Vec3f from_center = outside - center;
    const float a = chord.dot(chord);
    const float b = from_center.dot(chord);
    const float c = from_center.dot(from_center) - radius * radius;
    return outside + chord * ((-b - std::sqrt(std::max(b * b - a * c, 0.0f))) / a);
}

inline Vec3f pitchBetween(const Vec3f& eye, const Vec3f& a, const Vec3f& b) {
    const Vec3f to_a = a - eye;
    const Vec3f to_b = b - eye;
    const Vec3f middle = to_a.normalized() + to_b.normalized();
    return eye + middle.normalized() * static_cast<float>(std::max(to_a.length(), to_b.length()));
}

template <typename World>
bool pickThrough(const Vec3f& eye, const Vec3f& aim, Vec3f& hit, uint64_t& object) {
    const Vec3f ray = aim - eye;
    const auto length = static_cast<float>(ray.length());
    return length >= kRangeMargin && World::surface(eye, aim + ray / length * kPickPast, hit, object);
}

template <typename World>
bool readFacing(const Vec3f& eye, const Vec3f& point, float& normal_z) {
    const auto probe = [&](float offset, float& read) {
        Vec3f beside;
        uint64_t unused = 0;
        if (!pickThrough<World>(eye, Vec3f{point.x, point.y, point.z + offset}, beside, unused)) { return false; }
        read = normalZ(eye, point, beside);
        return true;
    };

    const bool below = probe(-kSlopeStep, normal_z);
    if (below && normal_z < kFloorNormal) { return true; }

    float above = 0.0f;
    if (!probe(kSlopeStep, above)) { return below; }
    normal_z = below ? std::min(normal_z, above) : std::min(above, kCeilingNormal);
    return true;
}

inline bool isStep(const Vec3f& eye, const Vec3f& outside, const Vec3f& inside) {
    return static_cast<float>((outside - eye).normalized().cross((inside - eye).normalized()).length()) < kSameRay;
}

template <typename World>
bool slideToMaxRange(
    const Query& q, const Vec3f& aim, float normal_z, float min_range, float max_range, GroundHit& out) {
    const auto rangeOf = [&](const Vec3f& point) { return static_cast<float>((point - q.caster).length()); };
    const float target = max_range - kCloseEnough * 0.5f;

    const float x = aim.x - q.caster.x;
    const float y = aim.y - q.caster.y;
    const bool down = normal_z > kCeilingNormal && aim.z > q.caster.z && x * x + y * y < target * target;

    Vec3f outside = aim;
    Vec3f inside;
    uint64_t object = 0;
    uint64_t inside_object = 0;
    bool bracketed = false;
    bool usable = false;
    bool was_past = true;
    bool stalled = false;
    for (int i = 0; i < kMaxTraces; ++i) {
        Vec3f next = outside;
        if (stalled) {
            next = pitchBetween(q.ray_start, outside, inside);
        } else if (bracketed) {
            next = chordCrossing(outside, inside, q.caster, target);
        } else {
            const float step = kMarchFactor * (rangeOf(outside) - target);
            next.z += down ? -step : step;
        }

        Vec3f hit;
        const bool landed = pickThrough<World>(q.ray_start, next, hit, object);
        if (!landed) {
            if (!bracketed) { return false; }
            hit = next + (next - q.ray_start).normalized() * kPickPast;
        }

        const bool behind = (hit - q.caster).dot(q.camera_forward) < 0.0f;
        const float dist = rangeOf(hit);
        const bool past = !landed || dist > max_range;
        stalled = bracketed && past == was_past;
        was_past = past;
        if (past) {
            if (behind) { return false; }
            outside = hit;
        } else {
            inside = hit;
            inside_object = object;
            bracketed = true;
            usable = !behind && dist >= min_range;
            if (usable && max_range - dist < kCloseEnough) {
                out = {.pos = hit, .object = object};
                return true;
            }
        }

        if (usable && isStep(q.ray_start, outside, inside)) {
            if ((outside - inside).length() > kMaxStep) { return false; }
            out = {.pos = inside, .object = inside_object};
            return true;
        }
    }
    return false;
}

template <typename World>
bool landAtMaxRange(const Query& q, const Vec3f& cursor_hit, float min_range, float max_range, GroundHit& out) {
    Vec3f aim;
    uint64_t object = 0;
    float normal_z = 0.0f;
    if (!pickThrough<World>(q.ray_start, cursor_hit, aim, object) || !readFacing<World>(q.ray_start, aim, normal_z)) {
        return dropToMaxRange<World>(q, cursor_hit, min_range, max_range, out);
    }

    const bool ceiling = normal_z <= kCeilingNormal;
    const bool trusted = !q.shown || static_cast<float>((aim - q.caster).length()) - max_range <= kReadReach;
    GroundHit drop;
    bool dropped = (!ceiling || !trusted) && dropToMaxRange<World>(q, cursor_hit, min_range, max_range, drop);
    if (normal_z >= kFloorNormal) {
        if (!dropped) { return false; }
        const bool same_floor = std::abs(drop.pos.z - aim.z) <= kLedge;
        if ((trusted && same_floor) || !q.shown || (drop.pos - *q.shown).length() <= kLedge) {
            out = drop;
            return true;
        }
    }

    GroundHit slide;
    if (slideToMaxRange<World>(q, aim, normal_z, min_range, max_range, slide)) {
        if (!dropped || !q.shown || (slide.pos - *q.shown).lengthSq() <= (drop.pos - *q.shown).lengthSq()) {
            out = slide;
            return true;
        }
    } else if (ceiling && trusted) {
        dropped = dropToMaxRange<World>(q, cursor_hit, min_range, max_range, drop);
    }
    out = drop;
    return dropped;
}

template <typename World>
bool pushToMinRange(const Query& q, const Vec3f& to_hit, float min_range, GroundHit& out) {
    const Vec3f point = q.caster + to_hit.normalized() * min_range;
    Vec3f ground;
    const Vec3f above{point.x, point.y, point.z + kMinRangeLift};
    const Vec3f below{point.x, point.y, point.z - kDropHeight};
    if (!World::pick(above, below, ground, out.object)) { return false; }

    out.pos = q.caster + (ground - q.caster).normalized() * min_range;
    Vec3f blocker;
    return !World::visible(q.ray_start, out.pos, blocker) || (blocker - out.pos).lengthSq() < kBlockedSq;
}

template <typename World>
bool clamp(const Query& q, const Vec3f& cursor_hit, GroundHit& out) {
    const float min_range = q.min_range + kRangeMargin;
    const float max_range = q.max_range - kRangeMargin;

    const Vec3f to_hit = cursor_hit - q.caster;
    const float dist_sq = to_hit.lengthSq();
    if (dist_sq > q.max_range * q.max_range) {
        const float reach = q.max_range + kClampReach;
        return dist_sq <= reach * reach && landAtMaxRange<World>(q, cursor_hit, min_range, max_range, out);
    }
    if (dist_sq <= kMinRangeDeadzoneSq || dist_sq >= q.min_range * q.min_range) { return false; }
    return pushToMinRange<World>(q, to_hit, min_range, out);
}
}  // namespace misc_ground_target_clamp
