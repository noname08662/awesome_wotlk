#pragma once

#include <xmmintrin.h>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>

#include "Lib/Storm.h"
#include "Map/CMapBaseObj.h"
#include "Map/CMapChunk.h"
#include "Math/Primitives.h"

class SceneLists;
struct TerrainHorizon;
struct DistanceBucketRow;
struct ScenePortalRect;

class CWFrustum {
public:
    Plane planes_[6];
    Vec3f corners_[8];  // near quad then far quad
    uint8_t _unkC0[52];
    TSLink<CWFrustum> link_;  // in a WMO group's frustums_

    // sphere
    HOOKKIT_HOOK(testSphere, 0x00983D20, hookkit::Conv::eThiscall, int, CWFrustum*, const AaSphere*);
    // box
    HOOKKIT_HOOK(testBox, 0x009839E0, hookkit::Conv::eThiscall, int, CWFrustum*, const AaBox*);
    // corners
    HOOKKIT_HOOK(setCorners, 0x00984240, hookkit::Conv::eThiscall, void, CWFrustum*, const Vec3f*);
};

static_assert(sizeof(CWFrustum) == 0xFC);

namespace world_scene {
inline constexpr float kFloorBias = 0.5f;  // 0x00ADF454

static_assert(std::bit_cast<uint32_t>(kFloorBias) == 0x3F000000);

enum RenderFlags : uint32_t {
    eRenderHorizon = 0x20,  // the terrain horizon is built and tested
    // draws the occluder polygons, and rasterized occluder edges go to drawn_occluder_edges_
    eRenderOccluderEdges = 0x2000,
    eRenderLiquid = 0x1000000,
};

inline auto& render_flags = *reinterpret_cast<RenderFlags*>(0x00CD774C);

inline bool renderHorizon() { return (render_flags & eRenderHorizon) != 0; }

inline auto& far_clip = *reinterpret_cast<float*>(0x00CD7748);
inline auto& render_counter = *reinterpret_cast<uint32_t*>(0x00CD87B0);  // once per render
// CMap's map id (game_ui::current_map_id is the UI's)
inline auto& current_map_id = *reinterpret_cast<int32_t*>(0x00ADFBC4);
// the chunks kept loaded
inline auto& chunk_loaded = *reinterpret_cast<ChunkRange*>(0x00CD77D8);

// a function, not an inline reference: MSVC /GL folds it everywhere
inline SceneLists& sceneLists() { return *reinterpret_cast<SceneLists*>(0x00CD9048); }

inline auto& terrain_horizon = *reinterpret_cast<TerrainHorizon*>(0x00CD87B8);
// a hole-free chunk raises the horizon only between them
inline auto& horizon_min_depth = *reinterpret_cast<float*>(0x00CD8780);
inline auto& horizon_max_depth = *reinterpret_cast<float*>(0x00CD8784);

// level 0 is the full-screen frustum, rebuilt every render; the window and portal frustums are pushed above it
inline auto& frustum_stack = *reinterpret_cast<CWFrustum (*)[32]>(0x00CDB168);
inline auto& frustum_level = *reinterpret_cast<int32_t*>(0x00CD8798);

inline CWFrustum& topFrustum() { return frustum_stack[frustum_level]; }

inline auto& screen_corners = *reinterpret_cast<Vec3f (*)[8]>(0x00CDB108);
inline auto& camera_frustum = *reinterpret_cast<CWFrustum*>(0x00CDD108);  // portals are clipped against it
inline auto& frustum_pool = *reinterpret_cast<TSExplicitList<CWFrustum>*>(0x00ADF6A8);

// setupCameraFrustum's outputs
inline auto& camera_pos = *reinterpret_cast<Vec3f*>(0x00CD8F5C);
inline auto& camera_target = *reinterpret_cast<Vec3f*>(0x00CD8F68);
inline auto& view_dir = *reinterpret_cast<Vec3f*>(0x00CD8F74);    // normalized camera_target - camera_pos
inline auto& view_plane = *reinterpret_cast<Plane*>(0x00CD8F80);  // normal view_dir, through camera_pos
// view_plane with the pitch taken out: the distance buckets are indexed by its depth
inline auto& horizontal_view_plane = *reinterpret_cast<Plane*>(0x00CD8F90);
// the frustum corners' box plus padding: exterior WMO groups are bucketed only when they intersect it
inline auto& frustum_corner_aabb = *reinterpret_cast<AaBox*>(0x00CD8F44);
// 2 when camera_target.x > camera_pos.x, + 1 when y is: the CMapChunk::kCornerVertex of the chunk corner nearest the
// camera. view_quadrant_opposite: the same with <, the far corner
inline auto& view_quadrant = *reinterpret_cast<int32_t*>(0x00CD8F38);
inline auto& view_quadrant_opposite = *reinterpret_cast<int32_t*>(0x00CD8F3C);
// frustum_corner_aabb in chunk indices
inline auto& cull_chunk_range = *reinterpret_cast<ChunkRange*>(0x00CDB0F4);
// pitch-free LookAt(horizontal view dir) * Translate(-camera_pos) * cull_projection
inline auto& horizon_xform = *reinterpret_cast<Mat4f*>(0x00ADF460);
inline auto& cull_projection = *reinterpret_cast<Mat4f*>(0x00ADF628);  // the device projection_
inline auto& cull_view = *reinterpret_cast<Mat4f*>(0x00ADF5E8);  // device matrix stack 10 top * Translate(-camera_pos)
inline auto& cull_view_proj = *reinterpret_cast<Mat4f*>(0x00ADF5A8);  // cull_view * cull_projection

// can point at freed defs outside the frame: CMap::Preload's loading pump presents after the area unload
inline auto& camera_map_obj_def = *reinterpret_cast<CMapObjDef**>(0x00CD87A4);  // null while the camera is outdoors
inline auto& camera_map_obj_def2 = *reinterpret_cast<CMapObjDef**>(0x00CD87A0);
inline auto& camera_groups = *reinterpret_cast<TSGrowableArray<uint32_t>*>(0x00CDB0D4);  // of camera_map_obj_def
inline auto& camera_groups2 = *reinterpret_cast<TSGrowableArray<uint32_t>*>(0x00CDB0E4);
inline auto& camera_wmo_skybox = *reinterpret_cast<void**>(0x00CD861C);  // the camera walk's CMapObj::skybox_
// a camera group lacks kGroupLitAsExterior
inline auto& camera_in_interior_group = *reinterpret_cast<uint32_t*>(0x00CD8620);

inline auto& exterior_rect = *reinterpret_cast<ScenePortalRect*>(0x00ADF570);
inline auto& outdoor_cull_rect = *reinterpret_cast<ScenePortalRect*>(0x00ADF58C);  // cullOutdoor's rect from inside
// what the walks append portal rects to: looking out from the camera, looking in from outdoors
inline auto& exterior_portal_rects = *reinterpret_cast<TSGrowableArray<ScenePortalRect>*>(0x00CDD0E8);
inline auto& entry_portal_rects = *reinterpret_cast<TSGrowableArray<ScenePortalRect>*>(0x00CDD0F8);

inline auto& entity_cull_done = *reinterpret_cast<uint32_t*>(0x00ADF3F0);  // 0 inside cullPortalEntities, 1 after
// a group with eGroupExteriorLit was visited, or unbucketed_exterior_groups_ wasn't empty
inline auto& exterior_group_in_view = *reinterpret_cast<uint32_t*>(0x00CD8778);
// every CMapEntity of the map
inline auto& map_entities = *reinterpret_cast<TSExplicitList<CMapEntity>*>(0x00AEEDC8);

// unk, scene time
HOOKKIT_HOOK_HANDLE(render, 0x0079A870, hookkit::Conv::eCdecl, void, uint32_t, float);
HOOKKIT_HOOK_HANDLE(pushFrustum, 0x00791950, hookkit::Conv::eCdecl, void);
HOOKKIT_HOOK_HANDLE(popFrustum, 0x0078FB50, hookkit::Conv::eCdecl, void);
// rect; both write the current level, setRectFrustum interpolates screen_corners
HOOKKIT_HOOK_HANDLE(setRectFrustum, 0x00790AF0, hookkit::Conv::eCdecl, void, const Rectf*);
// corners, rect
HOOKKIT_HOOK_HANDLE(setRectFrustumFromCorners, 0x00790E20, hookkit::Conv::eCdecl, void, const Vec3f*, const Rectf*);
// box; nonzero when it's outside the frustum on top of the stack
HOOKKIT_HOOK_HANDLE(cullBoxTopFrustum, 0x0078FB20, hookkit::Conv::eCdecl, int, const AaBox*);
// center, radius, flags
HOOKKIT_HOOK_HANDLE(testHorizonOcclusion, 0x0078FC40, hookkit::Conv::eCdecl, int, const Vec3f*, float, int);
// box, flags
HOOKKIT_HOOK_HANDLE(testHorizonOcclusionBox, 0x0078FDC0, hookkit::Conv::eCdecl, int, const AaBox*, int);
// rect
HOOKKIT_HOOK_HANDLE(appendExteriorPortalRect, 0x00795D00, hookkit::Conv::eCdecl, uint32_t, const ScenePortalRect*);
// rect
HOOKKIT_HOOK_HANDLE(appendEntryPortalRect, 0x00795D20, hookkit::Conv::eCdecl, uint32_t, const ScenePortalRect*);
// group index, def; map_obj::group_visitor while render's walks run
HOOKKIT_HOOK_HANDLE(visitPortalGroup, 0x00799310, hookkit::Conv::eCdecl, void, uint32_t, CMapObjDef*);
// entities, frustums, all, interior path; all: every linked entity, not only eInWmoGroup ones
HOOKKIT_HOOK_HANDLE(
    cullPortalEntities, 0x00793270, hookkit::Conv::eCdecl, void, TSExplicitList<CMapEntityLink>*, CWFrustum*, int, int);
// bucket; against the stack top frustum, the occluder volumes and the horizon
HOOKKIT_HOOK_HANDLE(cullOutdoorEntities, 0x00793060, hookkit::Conv::eCdecl, void, DistanceBucketRow*);
// fades fading_objects (0x00ADF1A8) and frees the entities of the expired ones
HOOKKIT_HOOK_HANDLE(updateFadingObjects, 0x00782F20, hookkit::Conv::eCdecl, void);
// rect; full screen outdoors, outdoor_cull_rect from inside
HOOKKIT_HOOK_HANDLE(cullOutdoor, 0x0079A790, hookkit::Conv::eCdecl, int, const Rectf*);
// bucket; raises the horizon by its terrain and low detail
HOOKKIT_HOOK_HANDLE(accumulateBucketHorizon, 0x00793760, hookkit::Conv::eCdecl, void, DistanceBucketRow*);
// drains culled_entities_ after the cull
HOOKKIT_HOOK_HANDLE(processCulledEntities, 0x00793450, hookkit::Conv::eCdecl, void);
// per visited group of the camera def or without kGroupOutdoor
HOOKKIT_HOOK_HANDLE(cullPortalGroups, 0x0079A260, hookkit::Conv::eCdecl, void);
// a CWFrustum from frustum_pool, or a new one; its contents are stale
HOOKKIT_HOOK_HANDLE(allocFrustum, 0x007983B0, hookkit::Conv::eCdecl, CWFrustum*);
// camera pos, camera target
HOOKKIT_HOOK_HANDLE(setupCameraFrustum, 0x00795400, hookkit::Conv::eCdecl, void, const Vec3f*, const Vec3f*);
// def, group
HOOKKIT_HOOK_HANDLE(bucketExteriorGroup, 0x00792AD0, hookkit::Conv::eCdecl, void, CMapObjDef*, CMapObjDefGroup*);
// head first; stops at the first group past the last bucket, which stays unlinked
HOOKKIT_HOOK_HANDLE(bucketUnbucketedExteriorGroups, 0x00792BD0, hookkit::Conv::eCdecl, void);
// the bucket of the entity's near corner, or culled_entities_ for a portal-only one
HOOKKIT_HOOK_HANDLE(bucketEntity, 0x00792E60, hookkit::Conv::eCdecl, void, CMapEntity*);

// a pushed frustum level, popped at scope exit; with corners and a rect, set to that rect's frustum
class FrustumScope {
public:
    FrustumScope() { pushFrustum{}(); }

    FrustumScope(std::span<const Vec3f, 8> corners, const Rectf& rect) : FrustumScope() {
        setRectFrustumFromCorners{}(corners.data(), &rect);
    }

    ~FrustumScope() { popFrustum{}(); }

    FrustumScope(const FrustumScope&) = delete;
    FrustumScope& operator=(const FrustumScope&) = delete;
};
}  // namespace world_scene

namespace map_low_detail {
// the occluder volumes, rebuilt from camera_pos and screen_corners at the start of every cullOutdoor
struct OccluderVolume {
    uint32_t first_plane;
    int32_t plane_count;
};

static_assert(sizeof(OccluderVolume) == 0x8);

inline auto& occluder_planes = *reinterpret_cast<TSGrowableArray<Plane>*>(0x00D2DCD8);
inline auto& occluder_volumes = *reinterpret_cast<TSGrowableArray<OccluderVolume>*>(0x00D2DCE8);

// the hard-coded occluder polygons the volumes are built from, keyed on world_scene::current_map_id
struct OccluderDef {
    enum Flags : uint32_t {
        // kept whole while the camera is indoors
        eUnclippedIndoors = 0x1,
        // also occludes shadow casters
        eShadowCaster = 0x2,
    };

    uint32_t map_id;
    Flags flags;
    AaBox bounds;
    Vec3f* verts;
    uint32_t vert_count;
};

static_assert(sizeof(OccluderDef) == 0x28);

inline auto& occluder_defs = *reinterpret_cast<OccluderDef (*)[62]>(0x00AF0040);

// the per-map occluder edge sets
struct EdgeSet {
    int32_t map_id;
    AaBox bounds;
    Vec3f* verts;
    int32_t vert_count;
};

static_assert(sizeof(EdgeSet) == 0x24);

inline const auto& kEdgeSets = *reinterpret_cast<const EdgeSet (*)[5]>(0x00D2DC20);

// sphere
HOOKKIT_HOOK_HANDLE(sphereInOccluderVolume, 0x007CCE00, hookkit::Conv::eCdecl, int, const AaSphere*);
HOOKKIT_HOOK_HANDLE(getOccluderCount, 0x007CCDF0, hookkit::Conv::eCdecl, int);
// verts, count; 1 when every vertex is behind every plane of one occluder volume
HOOKKIT_HOOK_HANDLE(polygonInOccluderVolume, 0x007CCFA0, hookkit::Conv::eCdecl, int, const Vec3f*, uint32_t);

inline bool polygonOccluded(const Vec3f* verts, uint32_t count) {
    return getOccluderCount{}() != 0 && polygonInOccluderVolume{}(verts, count) != 0;
}

inline bool sphereOccluded(const AaSphere& sphere) { return sphereInOccluderVolume{}(&sphere) != 0; }
}  // namespace map_low_detail

// a portal's screen rect in [0, 1]; also the union of the exterior portals seen from inside a WMO
struct ScenePortalRect {
    Rectf rect;
    float max_depth;  // < 0 while empty, -1 for entry portals
    Vec3f* verts;     // entry portals: their projected verts, in map_obj::entry_portal_verts
    uint32_t vert_count;

    // the per-frame reset of exterior_rect and outdoor_cull_rect
    static constexpr ScenePortalRect cleared() { return {Rectf::empty(), -1.0f, nullptr, 0}; }

    // the seed for a camera group with kGroupExteriorSeed
    static constexpr ScenePortalRect exteriorSeed() { return {Rectf::basic(), 0.0f, nullptr, 0}; }

    void unite(const ScenePortalRect& src) {
        rect = rect.merged(src.rect);
        max_depth = src.max_depth < max_depth ? max_depth : src.max_depth;
        vert_count = 0;
    }
};

static_assert(sizeof(ScenePortalRect) == 0x1C);

// a horizon occluder edge, cut at the bucket planes with one piece linked per bucket it spans
class CWorldOccluder {
public:
    Vec3f a_;
    Vec3f b_;
    TSLink<CWorldOccluder> link_;

    // back to the pool
    HOOKKIT_HOOK(release, 0x007CCA90, hookkit::Conv::eThiscall, void, CWorldOccluder*);
};

static_assert(sizeof(CWorldOccluder) == 0x20);

// one distance bucket
struct DistanceBucketRow {
    TSExplicitList<CMapChunk> chunks;            // by the vertex nearest the camera
    TSExplicitList<CMapObjDefGroup> wmo_groups;  // through CMapObjDefGroup::bucket_link_
    TSExplicitList<CMapEntity> entities;         // through CMapEntity::link_
    TSExplicitList<void> group_doodads;
    TSExplicitList<void> liquids;
    TSExplicitList<CWorldOccluder> low_detail;  // the occluder edges accumulateBucketHorizon rasterizes
    TSExplicitList<void> list6;
    // the kept chunks, in the bucket of the vertex farthest from the camera (a holed chunk in its own)
    TSExplicitList<CMapChunk> horizon_chunks;
    TSExplicitList<void> list8;
};

static_assert(sizeof(DistanceBucketRow) == 0x6C);

// render's distance buckets and the lists its cull hands on
class SceneLists {
public:
    // front to back by horizontal_view_plane depth * kBucketScale
    static constexpr int32_t kBucketCount = 64;
    static constexpr float kBucketDepth = 33.333332f;
    static constexpr float kBucketScale = 0.029999999f;  // 1 / kBucketDepth as the engine stores it

    // the bucketing drops anything past the last bucket
    static constexpr bool isBucket(int32_t n) { return n >= 0 && n < kBucketCount; }

    DistanceBucketRow buckets_[kBucketCount];
    // bucket i's near plane: normal = the horizontal view dir, through camera_pos + dir * kBucketDepth * i
    Plane bucket_planes_[kBucketCount];
    // the exterior groups of CMapObjDef::eDefObjectPlaced defs, through CMapObjDefGroup::bucket_link_, bucketed only
    // outdoors
    TSExplicitList<CMapObjDefGroup> unbucketed_exterior_groups_;
    uint8_t _unk1F0C[0x12C];
    // the group visitor's outputs
    TSExplicitList<CMapObjDefGroup> visited_portal_groups_;  // through CMapObjDefGroup::visited_link_
    TSExplicitList<CMapObjDefGroup> visited_liquid_groups_;  // through CMapObjDefGroup::liquid_link_
    TSExplicitList<CMapEntity> visible_entities_;            // the entity draw list
    // the entities the cull didn't reach, through CMapEntity::link_
    TSExplicitList<CMapEntity> culled_entities_;
    // with eRenderOccluderEdges, the occluder edges accumulateBucketHorizon rasterized
    TSExplicitList<CWorldOccluder> drawn_occluder_edges_;
    TSExplicitList<void> _unk2074;
    TSExplicitList<void> _unk2080;

    // dir, origin
    HOOKKIT_HOOK(
        setDistanceBucketPlanes, 0x007906C0, hookkit::Conv::eThiscall, void, SceneLists*, const Vec3f*, const Vec3f*);
    // in place of cullOutdoor when the camera's walk reached no exterior portal: drops every bucket's content
    HOOKKIT_HOOK(flushDistanceBuckets, 0x00794250, hookkit::Conv::eThiscall, void, SceneLists*);

    // horizon_max_depth, and its horizon_min_depth from inside a WMO
    static float maxDepth() {
        return static_cast<float>(
            static_cast<double>(world_scene::far_clip) - static_cast<double>(SceneLists::kBucketDepth));
    }

    static float minDepthInside(float exterior_depth) {
        return static_cast<float>(static_cast<double>(exterior_depth) + static_cast<double>(SceneLists::kBucketDepth));
    }
};

static_assert(offsetof(SceneLists, bucket_planes_) == 0x1B00);
static_assert(offsetof(SceneLists, unbucketed_exterior_groups_) == 0x1F00);
static_assert(offsetof(SceneLists, visited_portal_groups_) == 0x2038);
static_assert(offsetof(SceneLists, culled_entities_) == 0x205C);
static_assert(offsetof(SceneLists, drawn_occluder_edges_) == 0x2068);
static_assert(sizeof(SceneLists) == 0x208C);
static_assert(std::bit_cast<uint32_t>(SceneLists::kBucketDepth) == 0x42055555);
static_assert(std::bit_cast<uint32_t>(SceneLists::kBucketScale) == 0x3CF5C28F);

// the terrain horizon: per screen column, the highest terrain seen so far in horizon_xform's projected y, kColumnScale
// columns per unit of x; raised after each distance bucket, so a bucket is tested against the ones in front of it only
struct TerrainHorizon {
    static constexpr uint32_t kColumns = 384;
    static constexpr int32_t kCenter = 192;  // the column of x = 0
    static constexpr float kColumnScale = 64.0f;
    static constexpr float kMinPitch = -0.89999998f;  // view_dir.z
    static constexpr float kMaxPitch = 0.89999998f;
    static constexpr float kReset = -1000000.0f;          // every column, every render
    static constexpr float kCleared = -1000001.0f;        // a holed chunk's columns no occluder edge reached
    static constexpr float kOutdoorMinDepth = -10000.0f;  // horizon_min_depth with the camera outdoors
    // a sphere this small or anything nearer than kNearDepth is never hidden
    static constexpr float kRadiusEps = 2.3841858e-07f;
    static constexpr float kNearDepth = 50.0f;
    // the horizon strips skip a vertex this near, the edge bucketing drops an edge nearer than kEdgeMinDepth
    static constexpr float kStripMinZ = 0.027777778f;
    static constexpr float kEdgeMinDepth = 0.1f;
    static constexpr float kEdgeCutEps = 1e-5f;

    enum TestFlags : int32_t {
        eNoNearClip = 0x8,
    };

    enum TestResult : int32_t {
        eVisible = 0,
        eHidden = 2,
    };

    uint8_t low_detail_mask[kColumns];  // the columns an occluder edge reached; a holed chunk clears only the others
    float heights[kColumns];

    using ColumnHeights = std::span<const float, kColumns>;

    void reset() {
        std::ranges::fill(heights, kReset);
        std::memset(low_detail_mask, 0, sizeof(low_detail_mask));
    }

    static bool nearClipped(float z, int32_t flags) {
        if ((flags & eNoNearClip) != 0) { return false; }
        return kNearDepth > z;
    }

    static float scaled(float x) {
        return static_cast<float>(static_cast<double>(x) * static_cast<double>(kColumnScale));
    }

    static int scan(ColumnHeights heights, int32_t lo, int32_t hi, float height) {
        if (std::cmp_greater_equal(lo, kColumns) || hi < 0) { return 0; }
        lo = std::max(lo, 0);
        if (std::cmp_greater_equal(hi, kColumns)) { hi = kColumns - 1; }
        if (lo > hi) { return eHidden; }
        const __m128 h = _mm_set1_ps(height);
        int32_t i = lo;
        for (; i + 3 <= hi; i += 4) {
            if (_mm_movemask_ps(_mm_cmpgt_ps(h, _mm_loadu_ps(heights.subspan(i, 4).data()))) != 0) { return 0; }
        }
        for (; i <= hi; ++i) {
            if (height > heights[i]) { return 0; }
        }
        return eHidden;
    }
};

static_assert(sizeof(TerrainHorizon) == 0x780);
static_assert(std::bit_cast<uint32_t>(TerrainHorizon::kMinPitch) == 0xBF666666);
static_assert(std::bit_cast<uint32_t>(TerrainHorizon::kMaxPitch) == 0x3F666666);
static_assert(std::bit_cast<uint32_t>(TerrainHorizon::kReset) == 0xC9742400);
static_assert(std::bit_cast<uint32_t>(TerrainHorizon::kCleared) == 0xC9742410);
static_assert(std::bit_cast<uint32_t>(TerrainHorizon::kRadiusEps) == 0x34800000);
static_assert(std::bit_cast<uint32_t>(TerrainHorizon::kStripMinZ) == 0x3CE38E39);
static_assert(std::bit_cast<uint32_t>(TerrainHorizon::kEdgeMinDepth) == 0x3DCCCCCD);
static_assert(std::bit_cast<uint32_t>(TerrainHorizon::kEdgeCutEps) == 0x3727C5AC);
