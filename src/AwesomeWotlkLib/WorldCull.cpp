#include "WorldCull.h"

#include <hookkit/hookkit.h>

#include <algorithm>
#include <span>
#include <vector>

#include "Extensions.h"
#include "WorldCullCore.h"

#include "include/Map/CMapDoodadDef.h"
#include "include/Map/CMapEntity.h"
#include "include/Map/CMapObjDef.h"
#include "include/Map/WorldScene.h"

namespace world_cull::fixes {
namespace {
HOOKKIT_CALLSITE_HOOK(loads_call, 0x007B6B4F, CMapObjDef::updateGroupLoads, 5);
HOOKKIT_CALLSITE_HOOK(entity_call, 0x007B55C0, world_scene::bucketEntity);
using UnbucketedHook = world_scene::bucketUnbucketedExteriorGroups;
using GrowHook = CMapBaseObj::growOwnerBounds;

void bucketUnbucketedDetour() {
    while (true) {
        UnbucketedHook{}();
        if (world_scene::sceneLists().unbucketed_exterior_groups_.head() == nullptr) { break; }
    }
}

void bucketEntityDetour(CMapEntity* e) {
    if (e->isBareCarrier()) { e->cull_state_ = CMapPlaceableObj::eNotSceneTested; }
    world_scene::bucketEntity::viaDetour(e);
}

thread_local std::vector<CMapObjDefGroup*>* grown = nullptr;

constexpr int32_t kUnbucketedList = SceneLists::kBucketCount;
constexpr int32_t kLists = kUnbucketedList + 1;

TSExplicitList<CMapObjDefGroup>& listAt(int32_t at) {
    SceneLists& lists = world_scene::sceneLists();
    return at == kUnbucketedList ? lists.unbucketed_exterior_groups_ : lists.buckets_[at].wmo_groups;
}

void restoreLoopOrder(const std::array<bool, kLists>& touched) {
    ankerl::unordered_dense::map<const CMapObjDefGroup*, uint32_t> order;
    uint32_t i = 0;
    for (CMapObjDef* def : CMapObjDef::TSHT.fulllist) {
        for (CMapObjDefGroupLink* link : def->group_links_) {
            order.emplace(link->obj, i++);
        }
    }
    const auto rank = [&order](const CMapObjDefGroup* g) {
        const auto it = order.find(g);
        return it != order.end() ? it->second : UINT32_MAX;
    };
    std::vector<CMapObjDefGroup*> nodes;
    for (int32_t at = 0; at < kLists; ++at) {
        if (!touched[at]) { continue; }
        TSExplicitList<CMapObjDefGroup>& list = listAt(at);
        nodes.clear();
        for (CMapObjDefGroup* g : list) {
            nodes.push_back(g);
        }
        if (std::ranges::is_sorted(nodes, {}, rank)) { continue; }
        std::ranges::stable_sort(nodes, {}, rank);
        list.unlinkAll();
        for (CMapObjDefGroup* g : nodes) {
            list.linkToTail(g);
        }
    }
}

void rebucketGrown(std::vector<CMapObjDefGroup*>& groups) {
    std::ranges::sort(groups);
    groups.erase(std::ranges::unique(groups).begin(), groups.end());
    std::array<bool, kLists> touched = {};
    bool any = false;
    for (CMapObjDefGroup* g : groups) {
        CMapObjDef* def = g->def();
        if (!def->bucketsGroups() || def->root_ == nullptr || def->root_->loadedGroup(g->group_index_) == nullptr ||
            !world_scene::frustum_corner_aabb.intersects(g->bounds_)) {
            continue;
        }
        world_scene::bucketExteriorGroup::viaDetour(def, g);
        for (int32_t at = 0; at < kLists; ++at) {
            if (listAt(at).tail() == g) {
                touched[at] = true;
                any = true;
                break;
            }
        }
    }
    if (any) { restoreLoopOrder(touched); }
}

void loadsDetour(int bucket) {
    CMapObjDef::updateGroupLoads::viaDetour(bucket);
    std::vector<CMapObjDefGroup*> groups;
    grown = bucket != 0 ? &groups : nullptr;
    CMapDoodadDef::finishLoadedDoodads::viaDetour();
    grown = nullptr;
    if (!groups.empty()) { rebucketGrown(groups); }
}

void growDetour(const void* placeable, CMapBaseObj* owner) {
    GrowHook{}(placeable, owner);
    if (grown != nullptr && (owner->flags_ & CMapBaseObj::eKindWmoGroup) != 0) {
        grown->push_back(reinterpret_cast<CMapObjDefGroup*>(owner));
    }
}

void apply(bool on) {
    static_cast<void>(hookkit::HookTransaction::reinstallIf<bucketUnbucketedDetour>(UnbucketedHook{}, on));
    static_cast<void>(hookkit::HookTransaction::reinstallIf<bucketEntityDetour>(entity_call{}, on));
    static_cast<void>(hookkit::HookTransaction::reinstallIf<loadsDetour>(loads_call{}, on));
    static_cast<void>(hookkit::HookTransaction::reinstallIf<growDetour>(GrowHook{}, on));
}
}  // namespace
}  // namespace world_cull::fixes

namespace world_cull::replace {
namespace {
using CameraHook = CMapObj::traverseFromCamera_hook;
using ExteriorHook = CMapObj::traverseFromExterior_hook;
using SetupXformsHook = CMapObj::setupTraversalXforms_hook;
using ComputePortalRectHook = CMapObj::computePortalRect_hook;
using SphereHook = world_scene::testHorizonOcclusion;
using CameraGroups = TSGrowableArray<uint32_t>;

// the walk's Env on the engine's state and effects
class LiveEnv : public walk::Env {
public:
    LiveEnv()
        : walk::Env{
              {
                  .cache = map_obj::portal_rect_cache,
                  .stamp = map_obj::portal_rect_stamp,
                  .stamped_def = map_obj::stamped_def
              }
          } {}

    static void setInteriorPath(uint32_t value) { map_obj::walk_interior_path = value; }

    static void setSkybox(void* skybox) { world_scene::camera_wmo_skybox = skybox; }

    static void setInteriorGroup(uint32_t value) { world_scene::camera_in_interior_group = value; }

    static void visit(CMapObjDef*, uint32_t group) {
        if (map_obj::group_visitor != nullptr) { map_obj::group_visitor(group, map_obj::group_visitor_def); }
    }

    static void computePortalRect(
        const walk::Inputs&, CMapObj* root, const MapObjPortal* portal, PortalRectEntry* entry) {
        ComputePortalRectHook::viaDetour(root, portal, entry);
    }

    static void addExteriorRect(const ScenePortalRect& rect, bool also_outdoor) {
        world_scene::appendExteriorPortalRect::viaDetour(&rect);
        world_scene::exterior_rect.unite(rect);
        if (also_outdoor) { world_scene::outdoor_cull_rect.unite(rect); }
    }

    // the entry verts fix: the entry rects already appended follow the array when it moves
    static uint32_t appendEntryVerts(std::span<const Vec3f> verts) {
        TSGrowableArray<Vec3f>& array = map_obj::entry_portal_verts;
        const uint32_t first = array.count;
        const auto old_base = reinterpret_cast<uintptr_t>(array.data);
        array.setCount(first + verts.size());
        const auto new_base = reinterpret_cast<uintptr_t>(array.data);
        if (new_base != old_base && old_base != 0) {
            const uintptr_t old_end = old_base + sizeof(Vec3f) * first;
            for (ScenePortalRect& rect : world_scene::entry_portal_rects) {
                const auto at = reinterpret_cast<uintptr_t>(rect.verts);
                if (at >= old_base && at < old_end) {
                    rect.verts = reinterpret_cast<Vec3f*>(new_base + (at - old_base));
                }
            }
        }
        std::ranges::copy(verts, std::span(array).subspan(first).begin());
        return first;
    }

    static void appendEntryRect(const ScenePortalRect& rect, uint32_t first) {
        ScenePortalRect appended = rect;
        appended.verts = &map_obj::entry_portal_verts[first];
        world_scene::appendEntryPortalRect::viaDetour(&appended);
    }
};

walk::Inputs liveInputs(CMapObj* root, const Mat4f* local_to_world, const Mat4f* world_to_local,
    const Vec3f* camera_pos, const Vec3f* camera_target) {
    SetupXformsHook::viaDetour(root, local_to_world, world_to_local, camera_pos, camera_target);
    walk::Inputs in;
    in.camera_pos = world_scene::camera_pos;
    std::copy_n(std::begin(world_scene::camera_frustum.planes_), std::size(in.clip_planes), std::begin(in.clip_planes));
    in.view_proj = map_obj::traversal_xforms.view_proj;
    in.local_to_world = map_obj::traversal_xforms.local_to_world;
    in.camera = map_obj::traversal_camera;
    in.max_depth = map_obj::max_portal_depth;
    in.traversal_def = map_obj::traversal_def;
    in.xforms_ready = map_obj::xforms_ready;
    return in;
}

// the def2 seed fix. 0x79A870 walks def2 first (0xCDB0E4), then resets the rects (0x79A9A9) and walks the
// camera def (0xCDB0D4), whose seed (0x795D40, for camera groups with 0x40140) the reset
// erased: it goes back in before the walk's own rects, the engine's order without def2
void reseedExterior(const CMapObj* root, const CameraGroups& groups) {
    if (world_scene::camera_map_obj_def2 == nullptr || &groups != &world_scene::camera_groups) { return; }
    uint32_t flags = 0;
    for (const uint32_t group : groups) {
        if (group != 0xFFFF) { flags |= root->groupFlags(group); }
    }
    if ((flags & kGroupExteriorSeed) == 0) { return; }
    constexpr ScenePortalRect kSeed = ScenePortalRect::exteriorSeed();
    world_scene::exterior_rect.unite(kSeed);
    world_scene::appendExteriorPortalRect::viaDetour(&kSeed);
}

void cameraReplace(CMapObj* root, const Mat4f* local_to_world, const Mat4f* world_to_local, const Vec3f* camera_pos,
    const Vec3f* camera_target, const CameraGroups* groups) {
    const walk::Inputs in = liveInputs(root, local_to_world, world_to_local, camera_pos, camera_target);
    reseedExterior(root, *groups);
    LiveEnv env;
    walk::PortalWalk{root, in, env}.fromCamera(local_to_world, *groups);
}

void exteriorReplace(CMapObj* root, const Mat4f* local_to_world, const Mat4f* world_to_local, const Vec3f* camera_pos,
    const Vec3f* camera_target, const Rectf* parent_rect, uint32_t group) {
    const walk::Inputs in = liveInputs(root, local_to_world, world_to_local, camera_pos, camera_target);
    LiveEnv env;
    walk::PortalWalk{root, in, env}.fromExterior(group, *parent_rect);
}

int sphereReplace(const Vec3f* center, float radius, int flags) {
    const occlusion::Horizon horizon{
        .kXform = world_scene::horizon_xform,
        .kProjection = world_scene::cull_projection,
        .view_dir_z = world_scene::view_dir.z,
        .enabled = world_scene::renderHorizon()
    };
    return occlusion::horizonSphere(horizon, world_scene::terrain_horizon.heights, *center, radius, flags);
}

void apply(bool on) {
    if (on) { assertEnginePrecision(); }
    static_cast<void>(hookkit::HookTransaction::reinstallIf<cameraReplace>(CameraHook{}, on));
    static_cast<void>(hookkit::HookTransaction::reinstallIf<exteriorReplace>(ExteriorHook{}, on));
    static_cast<void>(hookkit::HookTransaction::reinstallIf<sphereReplace>(SphereHook{}, on));
}
}  // namespace
}  // namespace world_cull::replace

namespace world_cull {
void initialize(hookkit::HookTransaction&) {
    views::initialize();
    extensions::console::kCvarRegistry->add<int>({
        .name = "worldCullMode",
        .init = 1,
        .min{0},
        .max{1},
        .on_change = [](int value) {
            const bool on = value >= 1;
            fixes::apply(on);
            replace::apply(on);
            views::apply(on);
        }
    });
}
}  // namespace world_cull
