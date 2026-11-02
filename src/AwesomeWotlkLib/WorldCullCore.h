#pragma once

#include <emmintrin.h>
#include <hookkit/accessor.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <utility>
#include <vector>

#include "WorldCullX87.h"

#include "include/Constants.h"
#include "include/Map/CMapChunk.h"
#include "include/Map/CMapObjDef.h"
#include "include/Map/WorldScene.h"
#include "include/Math/Math.h"
#include "include/Math/Primitives.h"

#ifdef __clang__
#pragma clang fp contract(off)
#elifdef _MSC_VER
#pragma fp_contract(off)
#endif

namespace world_cull::bucket {
inline int32_t ofDepth(double depth) {
    return x87::floorIndex(static_cast<float>(depth * static_cast<double>(SceneLists::kBucketScale)));
}

inline int32_t index(const Plane& horizontal, const Vec3f& corner) {
    const double depth = x87::planeSide(horizontal, corner);
    return depth > static_cast<double>(0) ? ofDepth(depth) : 0;
}
}  // namespace world_cull::bucket

namespace world_cull::walk {
static_assert(std::bit_cast<uint32_t>(kEps4f) == 0x38D1B717, "the engine's 1e-4");
inline constexpr uint32_t kMaxProjectedVerts = sizeof(map_obj::project_portal_verts) / sizeof(Vec3f);
inline constexpr uint32_t kClipCapacity = kMaxProjectedVerts + 5 + 1;
using ClipBuffers = std::array<std::array<Vec3f, kClipCapacity>, 2>;

struct Inputs {
    Vec3f camera_pos;
    std::array<Plane, 5> clip_planes;
    Mat4f view_proj;
    Mat4f local_to_world;
    map_obj::TraversalCamera camera;
    uint32_t max_depth{};
    CMapObjDef* traversal_def{};
    uint32_t xforms_ready{};
    std::span<const Vec3f, 8> corners = world_scene::screen_corners;
    float thin_x = CMapObj::kThinPortalRect;
    float thin_y = CMapObj::kThinPortalRect;
};

inline map_obj::TraversalCamera traversalCamera(
    const Mat4f& world_to_local, const Vec3f& camera_pos, const Vec3f& camera_target) {
    map_obj::TraversalCamera out{};
    out.camera_local = x87::transformPoint(camera_pos, world_to_local);
    out.target_local = x87::transformPoint(camera_target, world_to_local);
    const Vec3f& cam = out.camera_local;
    const Vec3f& target = out.target_local;
    double x = static_cast<double>(target.x) - static_cast<double>(cam.x);
    double y = static_cast<double>(target.y) - static_cast<double>(cam.y);
    double z = static_cast<double>(target.z) - static_cast<double>(cam.z);
    double len_sq = z * z;
    len_sq += y * y;
    len_sq += x * x;
    if (static_cast<double>(kEps4f) < len_sq) {
        const double inv = static_cast<double>(1.0f) / std::sqrt(len_sq);
        x = x * inv;
        y = y * inv;
        z = inv * z;
    }
    out.view_dir_local = {static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)};
    double d = static_cast<double>(cam.z) * z;
    d += static_cast<double>(cam.y) * y;
    d += static_cast<double>(cam.x) * x;
    out.view_plane_d_local = static_cast<float>(-d);
    return out;
}

struct State {
    map_obj::PortalRectCache& cache;
    uint32_t& stamp;
    CMapObjDef*& stamped_def;

    void bump(CMapObjDef* def) const {
        if (def != stamped_def) {
            ++stamp;
            stamped_def = def;
        }
    }
};

struct Env {
    State state;

    static void setInteriorPath(uint32_t) {}

    static void setSkybox(void*) {}

    static void setInteriorGroup(uint32_t) {}

    static bool occluded(std::span<const Vec3f> verts) {
        return map_low_detail::polygonOccluded(verts.data(), verts.size());
    }

    static uint32_t appendEntryVerts(std::span<const Vec3f>) { return 0; }

    static void appendEntryRect(const ScenePortalRect&, uint32_t) {}
};

namespace detail {
inline std::span<Vec3f> clipToFrustum(
    std::span<const Vec3f> verts, std::span<const Plane, 5> planes, ClipBuffers& bufs) {
    std::ranges::copy(verts, std::span{bufs[0]}.first(verts.size()).begin());
    std::array<uint32_t, 2> counts{verts.size(), 0};
    std::span<Vec3f> out;
    std::array<int, kClipCapacity + 1> cls{};
    std::array<float, kClipCapacity + 1> dist{};
    for (uint32_t l = 0; l < 5; ++l) {
        const uint32_t src_index = l & 1;
        const uint32_t dst_index = (l - 1) & 1;
        const auto src = std::span<const Vec3f>(bufs[src_index]).first(counts[src_index]);
        const std::span dst{bufs[dst_index]};
        const uint32_t n = counts[src_index];
        const Plane& plane = planes[l];
        for (uint32_t i = 0; i < n; ++i) {
            double d = static_cast<double>(src[i].x) * static_cast<double>(plane.normal.x);
            d += static_cast<double>(src[i].z) * static_cast<double>(plane.normal.z);
            d += static_cast<double>(src[i].y) * static_cast<double>(plane.normal.y);
            d += static_cast<double>(plane.distance);
            dist[i] = static_cast<float>(d);
            cls[i] = d > static_cast<double>(kEps4f) ? 1 : (d < static_cast<double>(-kEps4f) ? 2 : 0);
        }
        cls[n] = cls[0];
        dist[n] = dist[0];

        uint32_t emitted = 0;
        const auto push = [&](const Vec3f& v) {
            if (emitted < kClipCapacity) { dst[emitted++] = v; }
        };
        for (uint32_t i = 0; i < n; ++i) {
            const Vec3f& cur = src[i];
            const int c = cls[i];
            if (c == 0) {
                push(cur);
                continue;
            }
            if (c == 1) { push(cur); }
            const int next_class = cls[i + 1];
            if (next_class != 0 && next_class != c) {
                const Vec3f& next = src[(i + 1) % n];
                const double t = static_cast<double>(dist[i]) /
                    (static_cast<double>(dist[i]) - static_cast<double>(dist[(i + 1) % n]));
                const auto lerp = [&](float a, float b) {
                    return static_cast<float>(
                        (static_cast<double>(b) - static_cast<double>(a)) * t + static_cast<double>(a));
                };
                push({lerp(cur.x, next.x), lerp(cur.y, next.y), lerp(cur.z, next.z)});
            }
        }
        if (emitted == 0) {
            out = {};
            break;
        }
        counts[dst_index] = emitted;
        out = dst.first(emitted);
    }
    return out;
}

template <typename Occluded>
uint16_t project(const Inputs& in, std::span<const Vec3f> local, const Vec3f& offset, bool skip_occluder,
    ClipBuffers& bufs, std::span<const Vec3f>& out, const Occluded& occluded) {
    out = {};
    local = local.first(std::min(local.size(), size_t{kMaxProjectedVerts}));
    std::array<Vec3f, kMaxProjectedVerts> world;
    for (size_t i = 0; i < local.size(); ++i) {
        const Vec3f moved{static_cast<float>(static_cast<double>(local[i].x) + static_cast<double>(offset.x)),
            static_cast<float>(static_cast<double>(local[i].y) + static_cast<double>(offset.y)),
            static_cast<float>(static_cast<double>(offset.z) + static_cast<double>(local[i].z))};
        world[i] = x87::transformPoint(moved, in.local_to_world);
    }
    const auto placed = std::span<const Vec3f>(world).first(local.size());
    if (!skip_occluder && occluded(placed)) { return PortalRectEntry::eEmpty; }

    const std::span<Vec3f> verts = clipToFrustum(placed, in.clip_planes, bufs);
    if (verts.size() < 3) { return PortalRectEntry::eEmpty; }
    for (Vec3f& p : verts) {
        const auto x = static_cast<float>(static_cast<double>(p.x) - static_cast<double>(in.camera_pos.x));
        const auto y = static_cast<float>(static_cast<double>(p.y) - static_cast<double>(in.camera_pos.y));
        const auto z = static_cast<float>(static_cast<double>(p.z) - static_cast<double>(in.camera_pos.z));
        std::array<float, 4> clip{};
        x87::transformClip(x, y, z, in.view_proj, clip);
        const float w = clip[3] < kEps4f ? kEps4f : clip[3];
        const double inv = static_cast<double>(1.0f) / static_cast<double>(w);
        p = {static_cast<float>(inv * static_cast<double>(clip[0])),
            static_cast<float>(inv * static_cast<double>(clip[1])), clip[2]};
    }
    out = verts;
    return 0;
}
}  // namespace detail

template <typename Occluded>
uint16_t computePortalRect(const Inputs& in, std::span<const Vec3f> portal_verts, const Plane& plane, uint16_t flags,
    Rectf& rect, const Occluded& occluded) {
    const Vec3f& cam = in.camera.camera_local;
    const double d = x87::planeSide(plane, cam);
    if (static_cast<double>(-CMapObj::kPortalPlaneEps) < d && d < static_cast<double>(CMapObj::kPortalPlaneEps) &&
        math::pointInPolygon(cam, portal_verts, plane.normal.dominantAxis())) {
        flags |= PortalRectEntry::eStraddles;
    }

    ClipBuffers bufs;
    std::span<const Vec3f> projected;
    if ((flags & PortalRectEntry::eStraddles) == 0) {
        constexpr Vec3f kNoOffset{0.0f, 0.0f, 0.0f};
        flags |= detail::project(
            in, portal_verts, kNoOffset, (flags & PortalRectEntry::eInteriorOnly) != 0, bufs, projected, occluded);
    }

    if ((flags & PortalRectEntry::eStraddles) != 0) {
        rect = Rectf::ndc();
    } else if ((flags & PortalRectEntry::eEmpty) != 0) {
        rect = Rectf::empty();
    } else {
        rect = Rectf::fromVerts(projected);
    }
    return flags;
}

template <typename Env>
class PortalWalk {
public:
    PortalWalk(CMapObj* root, const Inputs& in, Env& env) : root_(root), kIn(in), env_(env) {}

    void fromCamera(const Mat4f* local_to_world, const TSGrowableArray<uint32_t>& camera_groups) {
        from_camera_ = true;
        env_.setInteriorGroup(0);
        env_.state.bump(kIn.traversal_def);
        {
            const world_scene::FrustumScope scope;
            for (const uint32_t index : camera_groups) {
                const CMapObjGroup* group = root_->loadedGroup(index);
                if (group == nullptr) { continue; }
                if ((group->flags_ & kGroupLitAsExterior) == 0) { env_.setInteriorGroup(1); }
                traverse(index, MapObjPortalRef::kNoGroup, Rectf::ndc(), 0, 1);
            }
        }

        for (uint32_t i = 0; i < root_->group_count_; ++i) {
            const MapObjGroupInfo& info = root_->group_infos_[i];
            if ((info.flags & eGroupNoPortals) == 0) { continue; }
            const AaBox box = math::transformAaBox(*local_to_world, info.bounds);
            if (world_scene::cullBoxTopFrustum{}(&box) == 0 && root_->loadedGroup(i) != nullptr) {
                env_.visit(kIn.traversal_def, i);
            }
        }
    }

    void fromExterior(uint32_t index, const Rectf& parent_rect) {
        from_camera_ = false;
        env_.state.bump(kIn.traversal_def);
        traverse(index, MapObjPortalRef::kNoGroup, parent_rect, 0, 0);
    }

private:
    struct Candidate {
        ScenePortalRect rect;
        uint32_t first{};
    };

    void traverse(uint32_t index, uint32_t from_group, const Rectf& parent, uint32_t depth, uint32_t interior_path) {
        if (depth > kIn.max_depth) { return; }
        const CMapObjGroup* group = root_->loadedGroup(index);
        if (group == nullptr || (group->flags_ & eGroupNoPortals) != 0) { return; }
        if (interior_path != 0 && (group->flags_ & kGroupLitAsExterior) != 0) { interior_path = 0; }
        env_.setInteriorPath(interior_path);
        if (from_camera_ && (group->flags_ & eGroupSkybox) != 0) { env_.setSkybox(root_->skybox_); }
        env_.visit(kIn.traversal_def, index);
        if (group->portal_ref_count_ == 0) { return; }

        const bool exterior_root = !from_camera_ && depth == 0;
        if (exterior_root) {
            kBackfacing->clear();
            kCandidates->clear();
        }
        const MapObjPortalRef* ref = root_->portal_refs_ + group->portal_ref_start_;
        for (uint32_t r = 0; r < group->portal_ref_count_; ++r, ++ref) {
            const uint32_t target = ref->target_group;
            if (target == MapObjPortalRef::kNoGroup || target == from_group) { continue; }
            const MapObjPortal* portal = root_->portals_ + ref->portal_index;
            const uint32_t target_flags = root_->groupFlags(target);
            PortalRectEntry& entry = env_.state.cache[ref->portal_index];
            if (entry.stamp != env_.state.stamp) {
                entry.stamp = env_.state.stamp;
                entry.flags = 0;
                if ((target_flags & eGroupExterior) == 0 && (group->flags_ & eGroupExterior) == 0) {
                    entry.flags = PortalRectEntry::eInteriorOnly;
                }
                env_.computePortalRect(kIn, root_, portal, &entry);
            }

            if (facing(*portal, *ref) < static_cast<double>(0)) {
                if (exterior_root) {
                    kBackfacing->push_back(entry.rect.remapped<double>(Rectf::ndc(), Rectf::basic()));
                }
                continue;
            }
            if ((entry.flags & PortalRectEntry::eStraddles) == 0 && (entry.flags & PortalRectEntry::eEmpty) != 0) {
                continue;
            }
            const Rectf& e = entry.rect;
            if (parent.max_x < e.min_x || parent.min_x > e.max_x || parent.max_y < e.min_y || parent.min_y > e.max_y) {
                continue;
            }
            const Rectf clipped = e.clipped(parent);
            if (thin(clipped.min_x, clipped.max_x, kIn.thin_x) || thin(clipped.min_y, clipped.max_y, kIn.thin_y)) {
                continue;
            }

            if (from_camera_) {
                if ((target_flags & kGroupExteriorPortal) != 0) {
                    const uint32_t outdoor = target_flags & kGroupOutdoor;
                    addExteriorPortal(*portal, *ref, entry, outdoor != 0);
                    if (outdoor != 0) { continue; }
                }
            } else {
                if ((target_flags & kGroupOutdoor) != 0) { continue; }
                if (depth == 0 && kIn.xforms_ready != 0 && (target_flags & kGroupNoEntryPortal) == 0) {
                    addEntryPortal(*portal, *ref, entry);
                }
            }

            const Rectf child_rect{unit(clipped.min_y), unit(clipped.min_x), unit(clipped.max_y), unit(clipped.max_x)};
            world_scene::pushFrustum{}();
            world_scene::setRectFrustumFromCorners{}(kIn.corners.data(), &child_rect);
            traverse(target, index, clipped, depth + 1, interior_path);
            world_scene::popFrustum{}();
        }

        if (depth == 0 && !kCandidates->empty()) {
            for (const Candidate& c : *kCandidates) {
                const Rectf& r = c.rect.rect;
                const bool hidden =
                    std::ranges::any_of(*kBackfacing, [&r](const Rectf& b) { return r.intersects<false>(b); });
                if (!hidden) { env_.appendEntryRect(c.rect, c.first); }
            }
            kCandidates->clear();
            kBackfacing->clear();
        }
    }

    [[nodiscard]]
    double facing(const MapObjPortal& portal, const MapObjPortalRef& ref) const {
        const double d = x87::planeSide(portal.plane, kIn.camera.camera_local);
        return ref.side <= 0 ? -d : d;
    }

    static bool thin(float a, float b, float threshold) {
        return std::fabs(static_cast<double>(a) - static_cast<double>(b)) < static_cast<double>(threshold);
    }

    static float unit(float v) {
        const auto sum = static_cast<float>(static_cast<double>(v) + static_cast<double>(1.0f));
        return static_cast<float>(static_cast<double>(sum) / static_cast<double>(2.0f));
    }

    static Vec3f portalOffset(const MapObjPortal& portal, const MapObjPortalRef& ref) {
        const Vec3f offset = portal.plane.normal * CMapObj::kPortalPlaneEps;
        return ref.side > 0 ? -offset : offset;
    }

    bool projectRect(const MapObjPortal& portal, const MapObjPortalRef& ref, PortalRectEntry& entry,
        ScenePortalRect& out, std::span<const Vec3f>& verts) {
        const auto occluded = [this](std::span<const Vec3f> world) { return env_.occluded(world); };
        entry.flags |= detail::project(kIn, root_->portalVerts(portal), portalOffset(portal, ref),
            (entry.flags & PortalRectEntry::eInteriorOnly) != 0, bufs_, verts, occluded);
        if (verts.size() <= 2) { return false; }
        out.rect = Rectf::fromVerts(verts).remapped<double>(Rectf::ndc(), Rectf::basic());
        return true;
    }

    void addExteriorPortal(
        const MapObjPortal& portal, const MapObjPortalRef& ref, PortalRectEntry& entry, bool also_outdoor) {
        if ((entry.flags & PortalRectEntry::eExteriorAdded) != 0) { return; }
        entry.flags |= PortalRectEntry::eExteriorAdded;
        ScenePortalRect rect{};
        std::span<const Vec3f> verts;
        if (!projectRect(portal, ref, entry, rect, verts)) { return; }
        rect.max_depth = static_cast<float>(maxViewDepth(portal));
        env_.addExteriorRect(rect, also_outdoor);
    }

    void addEntryPortal(const MapObjPortal& portal, const MapObjPortalRef& ref, PortalRectEntry& entry) {
        if ((entry.flags & (PortalRectEntry::eExteriorAdded | PortalRectEntry::eEntryAdded)) != 0) { return; }
        ScenePortalRect rect{};
        std::span<const Vec3f> verts;
        if (projectRect(portal, ref, entry, rect, verts)) {
            rect.max_depth = -1.0f;
            rect.vert_count = verts.size();
            kCandidates->push_back({rect, env_.appendEntryVerts(verts)});
        }
        entry.flags |= PortalRectEntry::eEntryAdded;
    }

    [[nodiscard]]
    double maxViewDepth(const MapObjPortal& portal) const {
        const std::span<const Vec3f> v = root_->portalVerts(portal);
        const auto n = v.size();
        const auto r = [](float f) { return static_cast<double>(f); };
        const double dx = r(kIn.camera.view_dir_local.x);
        const double dy = r(kIn.camera.view_dir_local.y);
        const double dz = r(kIn.camera.view_dir_local.z);
        const double d = r(kIn.camera.view_plane_d_local);
        const uint32_t unrolled = n >= 4 ? (((n - 4) >> 2) + 1) * 4 : 0;
        double result = 0;
        for (uint32_t i = 0; i < n; ++i) {
            double s = r(v[i].y) * dy;
            if (i < unrolled && (i & 3) != 1) {
                s += r(v[i].z) * dz;
                s += r(v[i].x) * dx;
            } else {
                s += r(v[i].x) * dx;
                s += dz * r(v[i].z);
            }
            s += d;
            result = std::max(result, s);
        }
        return result;
    }

    CMapObj* root_;
    const Inputs& kIn;
    Env& env_;
    bool from_camera_ = false;
    ClipBuffers bufs_;
    struct BackfacingTag;
    static constexpr utils::Accessor<std::vector<Rectf>, BackfacingTag> kBackfacing{};
    struct CandidatesTag;
    static constexpr utils::Accessor<std::vector<Candidate>, CandidatesTag> kCandidates{};
};
}  // namespace world_cull::walk

namespace world_cull::occlusion {
struct Horizon {
    const Mat4f& kXform;
    const Mat4f& kProjection;
    float view_dir_z;
    bool enabled;

    [[nodiscard]]
    bool off() const {
        return !enabled || view_dir_z < TerrainHorizon::kMinPitch || view_dir_z > TerrainHorizon::kMaxPitch;
    }
};

struct Build {
    Horizon horizon;
    Vec3f camera_pos;
    Vec3f camera_target;
    float min_depth;
    float max_depth;
};

using ColumnHeights = std::span<const float, TerrainHorizon::kColumns>;

struct Buffer {
    std::span<float, TerrainHorizon::kColumns> heights;
    std::span<uint8_t, TerrainHorizon::kColumns> mask;
};

using Chunk = CMapChunk::Chunk;
using Edge = CMapChunk::Edge;
using VertexOffsets = CMapChunk::VertexOffsets;

namespace detail {
using RowIndices = std::array<uint32_t, CMapChunk::kOuterRowVertices>;
using RowPoints = std::array<Vec3f, CMapChunk::kOuterRowVertices>;

inline int32_t column(float scaled, int32_t offset) {
    return static_cast<int32_t>(static_cast<uint32_t>(x87::floorIndex(scaled)) + static_cast<uint32_t>(offset));
}

inline int scan(ColumnHeights heights, int32_t lo, int32_t hi, float height) {
    if (std::cmp_greater_equal(lo, TerrainHorizon::kColumns) || hi < 0) { return 0; }
    lo = std::max(lo, 0);
    if (std::cmp_greater_equal(hi, TerrainHorizon::kColumns)) { hi = TerrainHorizon::kColumns - 1; }
    if (lo > hi) { return TerrainHorizon::eHidden; }
    const __m128 h = _mm_set1_ps(height);
    int32_t i = lo;
    for (; i + 3 <= hi; i += 4) {
        if (_mm_movemask_ps(_mm_cmpgt_ps(h, _mm_loadu_ps(heights.subspan(i, 4).data()))) != 0) { return 0; }
    }
    for (; i <= hi; ++i) {
        if (height > heights[i]) { return 0; }
    }
    return TerrainHorizon::eHidden;
}

inline bool span(float x0, float x1, int32_t& lo, int32_t& hi) {
    lo = column(TerrainHorizon::scaled(x0), TerrainHorizon::kCenter);
    hi = column(TerrainHorizon::scaled(x1), TerrainHorizon::kCenter);
    if (hi < lo) { std::swap(lo, hi); }
    lo = std::max(lo, 0);
    if (std::cmp_greater_equal(hi, TerrainHorizon::kColumns)) { hi = TerrainHorizon::kColumns - 1; }
    return lo <= hi;
}

inline bool nearZ(float z) { return TerrainHorizon::kStripMinZ > z; }

inline void raise(const Vec3f& p0, const Vec3f& p1, Buffer buf, bool mark) {
    if (nearZ(p0.z) || nearZ(p1.z)) { return; }
    const float height = p1.y > p0.y ? p0.y : p1.y;
    int32_t lo = 0;
    int32_t hi = 0;
    if (!span(p0.x, p1.x, lo, hi)) { return; }
    if (mark) { std::ranges::fill(buf.mask.subspan(lo, static_cast<size_t>(hi - lo + 1)), uint8_t{1}); }
    const __m128 h = _mm_set1_ps(height);
    int32_t c = lo;
    for (; c + 3 <= hi; c += 4) {
        float* at = buf.heights.subspan(c, 4).data();
        _mm_storeu_ps(at, _mm_max_ps(h, _mm_loadu_ps(at)));
    }
    for (; c <= hi; ++c) {
        buf.heights[c] = std::max(height, buf.heights[c]);
    }
}

inline void clear(const Vec3f& p0, const Vec3f& p1, Buffer buf) {
    int32_t lo = 0;
    int32_t hi = 0;
    if (!span(p0.x, p1.x, lo, hi)) { return; }
    const __m128 cleared = _mm_set1_ps(TerrainHorizon::kCleared);
    const __m128i one = _mm_set1_epi32(1);
    const __m128i zero = _mm_setzero_si128();
    int32_t c = lo;
    for (; c + 3 <= hi; c += 4) {
        int32_t bytes = 0;
        std::memcpy(&bytes, buf.mask.subspan(c, 4).data(), sizeof(bytes));
        const __m128i mask = _mm_unpacklo_epi16(_mm_unpacklo_epi8(_mm_cvtsi32_si128(bytes), zero), zero);
        const __m128 keep = _mm_castsi128_ps(_mm_cmpeq_epi32(_mm_and_si128(mask, one), one));
        float* at = buf.heights.subspan(c, 4).data();
        const __m128 v = _mm_loadu_ps(at);
        _mm_storeu_ps(at, _mm_or_ps(_mm_and_ps(keep, v), _mm_andnot_ps(keep, cleared)));
    }
    for (; c <= hi; ++c) {
        if ((buf.mask[c] & 1) == 0) { buf.heights[c] = TerrainHorizon::kCleared; }
    }
}

inline void stripVertices(
    const x87::Lanes& xform, const Chunk& c, VertexOffsets offsets, const RowIndices& indices, RowPoints& p) {
    const __m128d origin_x = _mm_set1_pd(c.origin.x);
    const __m128d origin_y = _mm_set1_pd(c.origin.y);
    const __m128d origin_z = _mm_set1_pd(c.origin.z);
    for (uint32_t i = 0; i < CMapChunk::kOuterRowVertices; i += 2) {
        const uint32_t a = indices[i];
        const uint32_t b = indices[i + 1 < CMapChunk::kOuterRowVertices ? i + 1 : i];
        const __m128d x = x87::toFloat(_mm_add_pd(_mm_set_pd(offsets[b].x, offsets[a].x), origin_x));
        const __m128d y = x87::toFloat(_mm_add_pd(_mm_set_pd(offsets[b].y, offsets[a].y), origin_y));
        const __m128d z = x87::toFloat(_mm_add_pd(_mm_set_pd(c.heights[b], c.heights[a]), origin_z));
        std::array<Vec3f, 2> pair;
        xform.project2(x, y, z, pair);
        p[i] = pair[0];
        if (i + 1 < CMapChunk::kOuterRowVertices) { p[i + 1] = pair[1]; }
    }
}

inline void strip(
    const x87::Lanes& xform, Buffer buf, const Chunk& c, VertexOffsets offsets, const RowIndices& indices) {
    RowPoints p;
    stripVertices(xform, c, offsets, indices, p);
    for (uint32_t i = 0; i + 1 < CMapChunk::kOuterRowVertices; ++i) {
        if (c.holes != 0) {
            clear(p[i], p[i + 1], buf);
        } else {
            raise(p[i], p[i + 1], buf, false);
        }
    }
}

inline void addChunk(const Build& b, const x87::Lanes& xform, Buffer buf, const Chunk& c, VertexOffsets offsets) {
    RowIndices indices;
    const uint32_t row = CMapChunk::kCornerVertex[b.camera_pos.x > b.camera_target.x ? 2 : 0];
    for (uint32_t k = 0; k < CMapChunk::kOuterRowVertices; ++k) {
        indices[k] = row + k;
    }
    strip(xform, buf, c, offsets, indices);
    const uint32_t col = CMapChunk::kCornerVertex[b.camera_pos.y > b.camera_target.y ? 1 : 0];
    for (uint32_t k = 0; k < CMapChunk::kOuterRowVertices; ++k) {
        indices[k] = col + CMapChunk::kRowStride * k;
    }
    strip(xform, buf, c, offsets, indices);
}
}  // namespace detail

inline int horizonSphere(const Horizon& h, ColumnHeights heights, const Vec3f& center, float radius, int32_t flags) {
    if (h.off()) { return 0; }
    if (std::fabs(radius) < TerrainHorizon::kRadiusEps) { return 0; }
    const Vec3f p = x87::transformPoint(center, h.kXform);
    if (TerrainHorizon::nearClipped(p.z, flags)) { return 0; }
    const Vec3f o = x87::transformPoint({radius, radius, 0.0f}, h.kProjection);
    const double inv = static_cast<double>(1.0f) / static_cast<double>(p.z);
    const double rx = static_cast<double>(p.x) * inv;
    const double ox = static_cast<double>(o.x) * inv;
    const double oy = static_cast<double>(o.y) * inv;
    const double ry = inv * static_cast<double>(p.y);
    const auto top = static_cast<float>(oy + ry);
    const auto lo_scaled = static_cast<float>((rx - ox) * static_cast<double>(TerrainHorizon::kColumnScale));
    const auto hi_scaled =
        static_cast<float>((static_cast<double>(static_cast<float>(ox)) + static_cast<double>(static_cast<float>(rx))) *
            static_cast<double>(TerrainHorizon::kColumnScale));
    const int32_t lo = detail::column(lo_scaled, TerrainHorizon::kCenter);
    const int32_t hi = detail::column(hi_scaled, TerrainHorizon::kCenter + 1);
    return detail::scan(heights, lo, hi, top);
}

inline int horizonBox(
    const Horizon& h, const x87::Lanes& xform, ColumnHeights heights, const AaBox& box, int32_t flags) {
    if (h.off()) { return 0; }
    const double near_depth = (flags & TerrainHorizon::eNoNearClip) != 0
        ? -std::numeric_limits<double>::infinity()
        : static_cast<double>(TerrainHorizon::kNearDepth);
    std::array<double, 8> xs{};
    std::array<double, 8> ys{};
    if (!xform.projectBox(box, near_depth, xs, ys)) { return 0; }
    float min_x = FLT_MAX;
    auto min_x_st = static_cast<double>(min_x);
    float max_x = -FLT_MAX;
    float max_y = -FLT_MAX;
    for (int i = 0; i < 8; ++i) {
        const double x = xs[i];
        const double y = ys[i];
        if (static_cast<double>(min_x) > x) {
            min_x_st = x;
            min_x = static_cast<float>(x);
        } else {
            min_x_st = static_cast<double>(min_x);
        }
        if (static_cast<double>(max_x) < x) { max_x = static_cast<float>(x); }
        if (static_cast<double>(max_y) < y) { max_y = static_cast<float>(y); }
    }
    const auto lo_scaled = static_cast<float>(min_x_st * static_cast<double>(TerrainHorizon::kColumnScale));
    const auto hi_scaled =
        static_cast<float>(static_cast<double>(max_x) * static_cast<double>(TerrainHorizon::kColumnScale));
    const int32_t lo = detail::column(lo_scaled, TerrainHorizon::kCenter);
    const int32_t hi = detail::column(hi_scaled, TerrainHorizon::kCenter + 1);
    return detail::scan(heights, lo, hi, max_y);
}

inline void accumulate(const Build& b, const x87::Lanes& xform, Buffer buf, std::span<const Chunk> chunks,
    std::span<const Edge> edges, VertexOffsets offsets) {
    if (b.horizon.off()) { return; }
    for (const Chunk& c : chunks) {
        if (c.holes != 0) { continue; }
        if (b.max_depth > c.depth && b.min_depth < c.depth) { detail::addChunk(b, xform, buf, c, offsets); }
    }
    for (const Chunk& c : chunks) {
        if (c.holes != 0) { detail::addChunk(b, xform, buf, c, offsets); }
    }
    for (const auto& [a, b] : edges) {
        std::array<Vec3f, 2> p;
        xform.project2InFront(_mm_set_pd(b.x, a.x), _mm_set_pd(b.y, a.y), _mm_set_pd(b.z, a.z), p);
        detail::raise(p[0], p[1], buf, true);
    }
}

template <typename Emit>
void bucketEdge(const Vec3f& a, const Vec3f& b, float view_dir_z, const Plane& horizontal,
    std::span<const Plane, SceneLists::kBucketCount> planes, const Emit& emit) {
    if (view_dir_z < TerrainHorizon::kMinPitch || view_dir_z > TerrainHorizon::kMaxPitch) { return; }
    const auto r = [](float f) { return static_cast<double>(f); };
    const double da = x87::planeSide(horizontal, a);
    const double db = x87::planeSide(horizontal, b);
    const auto db_stored = static_cast<float>(db);
    if (r(TerrainHorizon::kEdgeMinDepth) > da && r(TerrainHorizon::kEdgeMinDepth) > db) { return; }
    const int32_t na = bucket::ofDepth(da);
    const int32_t nb = bucket::ofDepth(r(db_stored));
    if (na >= SceneLists::kBucketCount && nb >= SceneLists::kBucketCount) { return; }
    if (na == nb) {
        emit(na, a, b);
        return;
    }
    Vec3f near_end = na > nb ? b : a;
    const Vec3f far_end = na > nb ? a : b;
    int32_t lo = na > nb ? nb : na;
    int32_t hi = na > nb ? na : nb;
    lo = std::max(lo, 0);
    if (hi >= SceneLists::kBucketCount) { hi = SceneLists::kBucketCount - 1; }
    if (lo >= hi) {
        emit(lo, near_end, far_end);
        return;
    }
    for (int32_t k = lo; k < hi; ++k) {
        const Plane& p = planes[k + 1];
        const double df = x87::planeSide(p, far_end);
        const double dn = x87::planeSide(p, near_end);
        const double span = df - dn;
        Vec3f cut = far_end;
        if (r(TerrainHorizon::kEdgeCutEps) < span) {
            const double t = df / span;
            cut = {static_cast<float>(r(far_end.x) + (r(near_end.x) - r(far_end.x)) * t),
                static_cast<float>(r(far_end.y) + (r(near_end.y) - r(far_end.y)) * t),
                static_cast<float>(r(far_end.z) + t * (r(near_end.z) - r(far_end.z)))};
        }
        emit(k, near_end, cut);
        near_end = cut;
    }
    emit(hi, near_end, far_end);
}
}  // namespace world_cull::occlusion

namespace world_cull::view {
struct CameraView {
    Vec3f camera_pos;
    Vec3f camera_target;
    Vec3f view_dir;
    Plane view_plane;
    Plane horizontal_view_plane;
    Mat4f projection;
    Mat4f view_proj;
    std::array<Vec3f, 8> corners;
    CWFrustum frustum;
    AaBox corner_aabb;
    Mat4f horizon_xform;
    x87::Lanes horizon_lanes{};
    int32_t quadrant{};
    int32_t quadrant_opposite{};
    ChunkRange chunk_range{};
    std::array<Plane, SceneLists::kBucketCount> bucket_planes;

    walk::Inputs walkInputs(CMapObjDef* def, float thin_x, float thin_y) const {
        walk::Inputs in;
        in.camera_pos = camera_pos;
        std::copy_n(std::begin(frustum.planes_), in.clip_planes.size(), in.clip_planes.begin());
        in.view_proj = view_proj;
        in.local_to_world = def->transform_;
        in.camera = walk::traversalCamera(def->inv_transform_, camera_pos, camera_target);
        in.max_depth = map_obj::max_portal_depth;
        in.traversal_def = def;
        in.xforms_ready = 1;
        in.corners = corners;
        in.thin_x = thin_x;
        in.thin_y = thin_y;
        return in;
    }
};

namespace detail {
inline int32_t chunkIndex(float v) {
    return x87::floorIndex(static_cast<float>(-(static_cast<double>(v) - static_cast<double>(CMapChunk::kGridHalf)) *
        static_cast<double>(CMapChunk::kInvSize)));
}

inline void bucketPlanes(const Vec3f& dir, const Vec3f& origin, std::span<Plane, SceneLists::kBucketCount> out) {
    const auto r = [](float f) { return static_cast<double>(f); };
    auto at = static_cast<double>(0);
    for (Plane& p : out) {
        const double x = r(dir.x) * at + r(origin.x);
        const double y = at * r(dir.y) + r(origin.y);
        const double z = at * r(dir.z) + r(origin.z);
        const double d = (x * r(dir.x) + z * r(dir.z)) + y * r(dir.y);
        p = {dir, static_cast<float>(-d)};
        at = at + r(SceneLists::kBucketDepth);
    }
}
}  // namespace detail

inline void build(CameraView& out, const Vec3f& pos, const Vec3f& target, const Mat4f& view, const Mat4f& projection) {
    out.camera_pos = pos;
    out.camera_target = target;

    const double dx = static_cast<double>(target.x) - static_cast<double>(pos.x);
    const double dy = static_cast<double>(target.y) - static_cast<double>(pos.y);
    const double dz = static_cast<double>(target.z) - static_cast<double>(pos.z);
    double len_sq = dz * dz;
    len_sq = len_sq + dy * dy;
    len_sq = len_sq + dx * dx;
    const double inv = static_cast<double>(1.0f) / std::sqrt(len_sq);
    const double vx = dx * inv;
    const double vy = static_cast<double>(static_cast<float>(dy)) * inv;
    const double vz = static_cast<double>(static_cast<float>(dz)) * inv;
    out.view_dir = {static_cast<float>(vx), static_cast<float>(vy), static_cast<float>(vz)};

    const double h_len_sq = vy * vy + vx * vx;
    const auto h_len_sq_stored = static_cast<float>(h_len_sq);
    double hx = vx;
    double hy = vy;
    auto hz = static_cast<double>(0.0f);
    if (static_cast<double>(kEps4f) < h_len_sq) {
        const double h_inv = static_cast<double>(1.0f) / std::sqrt(h_len_sq);
        hx = vx * h_inv;
        hy = vy * h_inv;
        hz = static_cast<double>(0.0f) * h_inv;
    }
    const Vec3f horizontal{static_cast<float>(hx), static_cast<float>(hy), static_cast<float>(hz)};
    double h_d = hx * static_cast<double>(pos.x) + hy * static_cast<double>(pos.y);
    h_d = h_d + hz * static_cast<double>(pos.z);
    out.horizontal_view_plane = {horizontal, static_cast<float>(-h_d)};

    out.projection = projection;
    out.view_proj = math::multiply(view, projection);
    out.corners = math::calcFrustumCorners(view, projection);
    for (Vec3f& c : out.corners) {
        c = {static_cast<float>(static_cast<double>(c.x) + static_cast<double>(pos.x)),
            static_cast<float>(static_cast<double>(c.y) + static_cast<double>(pos.y)),
            static_cast<float>(static_cast<double>(c.z) + static_cast<double>(pos.z))};
    }
    out.frustum.setCorners(out.corners.data());

    out.corner_aabb = math::getAaBox(out.corners);
    out.chunk_range.y_min = detail::chunkIndex(out.corner_aabb.max.y);
    out.chunk_range.x_min = detail::chunkIndex(out.corner_aabb.max.x);
    out.chunk_range.y_max = detail::chunkIndex(out.corner_aabb.min.y);
    out.chunk_range.x_max = detail::chunkIndex(out.corner_aabb.min.x);
    out.quadrant = (target.x > pos.x ? 2 : 0) + (target.y > pos.y ? 1 : 0);
    out.quadrant_opposite = (target.x < pos.x ? 2 : 0) + (target.y < pos.y ? 1 : 0);
    detail::bucketPlanes(horizontal, pos, out.bucket_planes);

    double d = static_cast<double>(pos.x) * static_cast<double>(out.view_dir.x) +
        static_cast<double>(pos.y) * static_cast<double>(out.view_dir.y);
    d = d + static_cast<double>(out.view_dir.z) * static_cast<double>(pos.z);
    out.view_plane = {out.view_dir, static_cast<float>(-d)};

    if (!(kEps4f < h_len_sq_stored)) {
        out.horizon_xform = Mat4f::identity();
    } else {
        constexpr Vec3f kEye{0.0f, 0.0f, 0.0f};
        constexpr Vec3f kUp{0.0f, 0.0f, 1.0f};
        const Vec3f move{-pos.x, -pos.y, -pos.z};
        Mat4f look = math::createLookAt(kEye, horizontal, kUp);
        math::translate(look, move);
        out.horizon_xform = math::multiply(look, out.projection);
    }
    out.horizon_lanes.broadcast(out.horizon_xform);
}
}  // namespace world_cull::view
