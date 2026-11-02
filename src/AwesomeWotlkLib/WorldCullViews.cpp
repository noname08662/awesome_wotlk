#include <hookkit/accessor.h>

#include <algorithm>
#include <array>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <vector>

#include "Extensions.h"
#include "WorldCull.h"
#include "WorldCullCore.h"

#include "include/Graphics/CGxDevice.h"
#include "include/Map/CMapChunk.h"
#include "include/Map/CMapEntity.h"

#ifdef __clang__
#pragma clang fp contract(off)
#elifdef _MSC_VER
#pragma fp_contract(off)
#endif

namespace world_cull::views {
namespace {
struct View {
    Id id = 0;
    Spec spec;
    ankerl::unordered_dense::map<const CMapEntity*, guid_t> drawn;
};

constexpr auto kSideColumns =
    std::to_array<std::array<int8_t, 3>>({{1, 2, 3}, {-1, 2, -3}, {3, 2, -1}, {-3, 2, 1}, {1, -3, 2}, {1, 3, -2}});
static_assert(std::size(kSideColumns) == static_cast<size_t>(Side::eDown) + 1);

constexpr int column(int8_t c) { return (c < 0 ? -c : c) - 1; }

static_assert(std::ranges::all_of(kSideColumns,
                  [](const auto& s) {
                      const int a = column(s[0]);
                      const int b = column(s[1]);
                      const int c = column(s[2]);
                      const int flips = (a > b) + (a > c) + (b > c) + (s[0] < 0) + (s[1] < 0) + (s[2] < 0);
                      return ((1 << a) | (1 << b) | (1 << c)) == 7 && flips % 2 == 0;
                  }),
    "a side that mirrors the view would flip SetCorners' plane winding");

struct Frame {
    Vec3f pos;
    Mat4f view;
    Mat4f projection;

    [[nodiscard]]
    bool turnable(const Vec3f& view_dir) const {
        const auto zero = [](const Mat4f& mat, std::initializer_list<int> at) {
            return std::ranges::all_of(at, [&mat](int i) { return mat.data[i] == 0.0f; });
        };
        const auto& p = projection.data;
        const auto& v = view.data;
        if (!zero(projection, {1, 2, 3, 4, 6, 7, 8, 9, 12, 13}) || p[11] != 1.0f || p[15] != 0.0f || p[10] == 1.0f ||
            !zero(view, {3, 7, 11, 12, 13, 14}) || v[15] != 1.0f) {
            return false;
        }
        const double dot = static_cast<double>(v[2]) * view_dir.x + static_cast<double>(v[6]) * view_dir.y +
            static_cast<double>(v[10]) * view_dir.z;
        return dot > 1.0 - 1e-4;
    }

    [[nodiscard]]
    Rectf window(const Spec& spec) const {
        if (!spec.screen) { return spec.rect; }
        const Vec2f unit = screenUnit(spec.side);
        const Rectf& r = spec.rect;
        return {r.bottom * unit.y, r.left * unit.x, r.top * unit.y, r.right * unit.x};
    }

private:
    [[nodiscard]]
    Vec2f screenUnit(Side side) const {
        const float cot_x = projection.m[0][0];
        const float cot_y = projection.m[1][1];
        const float aspect = cot_y / cot_x;
        switch (side) {
            case Side::eUp:
            case Side::eDown:
                return {aspect, cot_y};
            case Side::eLeft:
            case Side::eRight:
                return {cot_x, 1.0f / aspect};
            case Side::eFront:
            case Side::eBack:
            default:
                return {1.0f / cot_x, 1.0f / cot_y};
        }
    }
};

struct Turn {
    static constexpr double kTargetDistance = 1024.0;

    Mat4f view;
    Mat4f projection;
    Vec3f target;
    float thin_x;
    float thin_y;

    Turn(const Frame& frame, const Spec& spec) : view(frame.view), projection(frame.projection) {
        const auto& s = kSideColumns[static_cast<size_t>(spec.side)];
        for (int row = 0; row < 4; ++row) {
            for (int k = 0; k < 3; ++k) {
                const float m = frame.view.m[row][column(s[k])];
                view.m[row][k] = s[k] < 0 ? -m : m;
            }
        }
        const Rectf r = frame.window(spec);
        const float w = r.width();
        const float h = r.height();
        projection.m[0][0] = 2.0f / w;
        projection.m[1][1] = 2.0f / h;
        projection.m[2][0] = -(r.right + r.left) / w;
        projection.m[2][1] = -(r.top + r.bottom) / h;
        const Vec3f& pos = frame.pos;
        target = {static_cast<float>(static_cast<double>(pos.x) + kTargetDistance * view.m[0][2]),
            static_cast<float>(static_cast<double>(pos.y) + kTargetDistance * view.m[1][2]),
            static_cast<float>(static_cast<double>(pos.z) + kTargetDistance * view.m[2][2])};
        thin_x = static_cast<float>(0.001 * projection.m[0][0] / frame.projection.m[0][0]);
        thin_y = static_cast<float>(0.001 * projection.m[1][1] / frame.projection.m[1][1]);
    }
};

template <typename T>
class Rows {
public:
    void add(int32_t n, const T& item) {
        if (n >= 0 && n <= last_) { rows_[n].push_back(item); }
    }

    void clear(int32_t last) {
        for (std::vector<T>& row : rows_) {
            row.clear();
        }
        last_ = last;
    }

    std::span<const T> operator[](int32_t n) const { return rows_[n]; }

private:
    std::array<std::vector<T>, SceneLists::kBucketCount> rows_;
    int32_t last_ = -1;
};

struct Scene {
    explicit Scene(view::CameraView& camera) : camera(camera) {}

    view::CameraView& camera;
    Rows<CMapObjDefGroup*> groups;
    Rows<CMapEntity*> entities;
    Rows<CMapChunk*> chunks;
    Rows<occlusion::Edge> edges;
    std::vector<CMapObjDefGroup*> unbucketed;
    ankerl::unordered_dense::set<CMapEntity*> wanted;
    int32_t last_row = -1;

    bool pick(std::span<CMapEntity* const> entities_wanted) {
        constexpr float kSeamMargin = 1.0f;
        wanted.clear();
        int32_t last = 0;
        for (CMapEntity* e : entities_wanted) {
            const AaSphere reach{e->prev_sphere_.position, e->prev_sphere_.radius + kSeamMargin};
            if (camera.frustum.testSphere(&reach) != 0) {
                wanted.insert(e);
                last = std::max(last, bucketOf(e->prev_bbox_));
            }
        }
        last_row = std::min(last + 1, SceneLists::kBucketCount - 1);
        return !wanted.empty();
    }

    void bucket(bool culls_outdoor, bool horizon_on) {
        groups.clear(last_row);
        entities.clear(last_row);
        chunks.clear(last_row);
        edges.clear(last_row);
        unbucketed.clear();
        bucketGroups(culls_outdoor, culls_outdoor && horizon_on);
        if (!culls_outdoor) { return; }
        bucketEntities();
        if (world_scene::camera_map_obj_def == nullptr) {
            for (CMapObjDefGroup* g : unbucketed) {
                groups.add(bucketOf(g->bounds_), g);
            }
        }
        if (horizon_on) {
            bucketChunks();
            bucketEdgeSets();
        }
    }

private:
    [[nodiscard]]
    int32_t bucketOf(const AaBox& box) const {
        return bucket::index(camera.horizontal_view_plane, box.nearCorner(camera.camera_pos, camera.camera_target));
    }

    void cut(const Vec3f& a, const Vec3f& b) {
        occlusion::bucketEdge(a, b, camera.view_dir.z, camera.horizontal_view_plane, camera.bucket_planes,
            [this](int32_t n, const Vec3f& p, const Vec3f& q) { edges.add(n, {.a = p, .b = q}); });
    }

    void bucketGroups(bool culls_outdoor, bool cuts_edges) {
        for (CMapObjDef* def : CMapObjDef::TSHT.fulllist) {
            const CMapObj* root = def->root_;
            const bool placed = def->objectPlaced();
            if (!def->bucketsGroups() || root == nullptr || (!culls_outdoor && !placed)) { continue; }
            for (CMapObjDefGroupLink* link : def->group_links_) {
                CMapObjDefGroup* g = link->obj;
                const CMapObjGroup* group = root->loadedGroup(g->group_index_);
                if (group == nullptr) { continue; }
                if (cuts_edges) {
                    for (const MapObjAntiportalEdge* e : group->antiportal_edges_) {
                        cut(x87::transformPoint(e->a, def->transform_), x87::transformPoint(e->b, def->transform_));
                    }
                }
                if (!camera.corner_aabb.intersects(g->bounds_) ||
                    (root->groupFlags(g->group_index_) & kGroupOutdoor) == 0) {
                    continue;
                }
                if (placed) {
                    unbucketed.push_back(g);
                } else {
                    groups.add(bucketOf(g->bounds_), g);
                }
            }
        }
    }

    void bucketEntities() {
        for (CMapEntity* e : wanted) {
            if (!e->portalOnly()) { entities.add(bucketOf(e->prev_bbox_), e); }
        }
    }

    void bucketChunks() {
        const auto [x_min, y_min, x_max, y_max] = world_scene::chunk_loaded.areas();
        for (int32_t ax = x_min; ax <= x_max; ++ax) {
            for (int32_t ay = y_min; ay <= y_max; ++ay) {
                const CMapArea* area = CMapArea::kMapAreas[ax][ay];
                if (area == nullptr || area->loading()) { continue; }
                for (int32_t block = 0; block < 16; ++block) {
                    bucketBlock(*area, area->chunk_x_ + (block >> 2) * 4, area->chunk_y_ + (block & 3) * 4);
                }
            }
        }
    }

    void bucketBlock(const CMapArea& area, int32_t cx0, int32_t cy0) {
        const ChunkRange block{.x_min = cx0, .y_min = cy0, .x_max = cx0 + 3, .y_max = cy0 + 3};
        if (!block.intersects(camera.chunk_range) || blockRow(cx0, cy0) > last_row) { return; }
        const auto [x_min, y_min, x_max, y_max] = block.clipped(world_scene::chunk_loaded);
        for (int32_t cx = x_min; cx <= x_max; ++cx) {
            for (int32_t cy = y_min; cy <= y_max; ++cy) {
                CMapChunk* c = area.chunkAt(cx, cy);
                if (c != nullptr && camera.frustum.testBox(&c->bounds_) != 0) {
                    chunks.add(bucket::index(camera.horizontal_view_plane, c->chunkCorner(camera.quadrant)), c);
                }
            }
        }
    }

    [[nodiscard]]
    int32_t blockRow(int32_t cx0, int32_t cy0) const {
        const auto edge = [](int32_t c) { return CMapChunk::kGridHalf - static_cast<float>(c) * CMapChunk::kSize; };
        const AaBox box{{edge(cx0 + 4), edge(cy0 + 4), 0.0f}, {edge(cx0), edge(cy0), 0.0f}};
        return bucket::index(camera.horizontal_view_plane, box.nearCorner(camera.camera_pos, camera.camera_target));
    }

    void bucketEdgeSets() {
        for (const auto& [map_id, bounds, verts, vert_count] : map_low_detail::kEdgeSets) {
            if (map_id != world_scene::current_map_id || camera.frustum.testBox(&bounds) == 0) { continue; }
            const std::span<const Vec3f> points{verts, static_cast<size_t>(vert_count)};
            for (size_t k = 0; k < points.size(); k += 2) {
                cut(points[k], points[k + 1]);
            }
        }
    }
};

class Visits {
public:
    struct Visit {
        CMapObjDefGroup* group{};
        std::vector<CWFrustum> chain;
    };

    void clear() {
        index_.clear();
        used_ = 0;
    }

    void add(CMapObjDefGroup* g, const CWFrustum& top) {
        const auto [it, inserted] = index_.try_emplace(g, used_);
        if (inserted) {
            if (used_ == slots_.size()) { slots_.emplace_back(); }
            auto& [group, chain] = slots_[used_++];
            group = g;
            chain.clear();
        }
        std::vector<CWFrustum>& chain = slots_[it->second].chain;
        chain.push_back(top);
        chain.back().link_ = {};
    }

    std::span<CWFrustum> chainOf(const CMapObjDefGroup* g) {
        const auto it = index_.find(g);
        if (it == index_.end()) { return {}; }
        return slots_[it->second].chain;
    }

    std::span<Visit> all() { return std::span(slots_).first(used_); }

private:
    std::vector<Visit> slots_;
    uint32_t used_ = 0;
    ankerl::unordered_dense::map<const CMapObjDefGroup*, uint32_t> index_;
};

struct WalkState {
    map_obj::PortalRectCache cache;
    uint32_t stamp = 0;
    CMapObjDef* stamped_def = nullptr;
    ScenePortalRect outdoor_cull_rect{};

    walk::State refs() { return {.cache = cache, .stamp = stamp, .stamped_def = stamped_def}; }
};

class Pass {
public:
    void run(View& view, const Frame& frame, std::span<CMapEntity* const> wanted) {
        view.drawn.clear();
        if (wanted.empty()) { return; }
        const Turn turn{frame, view.spec};
        view_ = &view;
        view::build(camera_, frame.pos, turn.target, turn.view, turn.projection);
        if (!scene_.pick(wanted)) { return; }
        horizon_.view_dir_z = camera_.view_dir.z;
        horizon_.enabled = view.spec.horizon && world_scene::renderHorizon();
        thin_x_ = turn.thin_x;
        thin_y_ = turn.thin_y;
        cull();
    }

private:
    class Env;

    [[nodiscard]]
    bool horizonOn() const {
        return !horizon_.off();
    }

    [[nodiscard]]
    bool hidden(const AaBox& box, int32_t flags) const {
        return occlusion::horizonBox(horizon_, camera_.horizon_lanes, own_horizon_.heights, box, flags) != 0;
    }

    walk::Inputs inputsFor(CMapObjDef* def) const { return camera_.walkInputs(def, thin_x_, thin_y_); }

    void visit(CMapObjDef* def, uint32_t group) { visits_.add(def->group(group), world_scene::topFrustum()); }

    void markDrawn(CMapEntity* e) const { view_->drawn.emplace(e, e->owner_guid_); }

    void cull();
    void cameraWalk(CMapObjDef* def, const TSGrowableArray<uint32_t>& camera_groups);
    void traverseGroupFromExterior(CMapObjDefGroup* g, const Rectf& rect);
    void cullOutdoor(const Rectf& rect, float min_depth);
    void cullChunks(CWFrustum& top, int32_t n);
    void cullOutdoorWmos(CWFrustum& top, int32_t n, const Rectf& rect);
    void cullOutdoorEntities(CWFrustum& top, int32_t n) const;
    void cullInteriorGroups(bool exterior_view);
    void cullPortalGroups();
    void cullPortalEntities(const CMapObjDefGroup* g, std::span<CWFrustum> chain, bool all) const;

    View* view_ = nullptr;
    view::CameraView camera_{};
    occlusion::Horizon horizon_{
        .kXform = camera_.horizon_xform,
        .kProjection = camera_.projection,
        .view_dir_z = 0.0f,
        .enabled = false
    };
    float thin_x_ = 0.0f;
    float thin_y_ = 0.0f;
    bool occluders_built_ = false;
    Scene scene_{camera_};
    Rows<occlusion::Chunk> queue_;
    Visits visits_;
    WalkState walk_{};
    TerrainHorizon own_horizon_ = {};
};

class Pass::Env : public walk::Env {
public:
    explicit Env(Pass& pass) : walk::Env{pass.walk_.refs()}, pass_(pass) {}

    void visit(CMapObjDef* def, uint32_t group) const { pass_.visit(def, group); }

    [[nodiscard]]
    bool occluded(std::span<const Vec3f> verts) const {
        return pass_.occluders_built_ && walk::Env::occluded(verts);
    }

    void computePortalRect(
        const walk::Inputs& in, CMapObj* root, const MapObjPortal* portal, PortalRectEntry* entry) const {
        entry->flags = walk::computePortalRect(in, root->portalVerts(*portal), portal->plane, entry->flags, entry->rect,
            [this](std::span<const Vec3f> verts) { return occluded(verts); });
    }

    void addExteriorRect(const ScenePortalRect& rect, bool also_outdoor) const {
        if (also_outdoor) { pass_.walk_.outdoor_cull_rect.unite(rect); }
    }

private:
    Pass& pass_;
};

void Pass::cull() {
    visits_.clear();
    ++walk_.stamp;
    walk_.stamped_def = nullptr;
    occluders_built_ = false;
    const world_scene::FrustumScope base;
    world_scene::topFrustum().setCorners(camera_.corners.data());

    if (world_scene::camera_map_obj_def == nullptr) {
        scene_.bucket(true, horizonOn());
        cullOutdoor(Rectf::basic(), TerrainHorizon::kOutdoorMinDepth);
    } else {
        ScenePortalRect& exterior = walk_.outdoor_cull_rect;
        exterior = ScenePortalRect::cleared();
        if (CMapObjDef* second = world_scene::camera_map_obj_def2) {
            cameraWalk(second, world_scene::camera_groups2);
            exterior = ScenePortalRect::cleared();
        }
        cameraWalk(world_scene::camera_map_obj_def, world_scene::camera_groups);
        const bool culls_outdoor = !(exterior.max_depth < 0.0f);
        scene_.bucket(culls_outdoor, horizonOn());
        if (culls_outdoor) { cullOutdoor(exterior.rect, SceneLists::minDepthInside(exterior.max_depth)); }
        cullInteriorGroups(culls_outdoor);
    }
    cullPortalGroups();
}

void Pass::cameraWalk(CMapObjDef* def, const TSGrowableArray<uint32_t>& camera_groups) {
    const walk::Inputs in = inputsFor(def);
    Env env{*this};
    walk::PortalWalk{def->root_, in, env}.fromCamera(&def->transform_, camera_groups);
}

void Pass::traverseGroupFromExterior(CMapObjDefGroup* g, const Rectf& rect) {
    CMapObjDef* def = g->def();
    CMapObj* root = def->root_;
    const uint32_t index = g->group_index_;
    const world_scene::FrustumScope scope{camera_.corners, rect};
    if (world_scene::cullBoxTopFrustum{}(&g->bounds_) != 0) { return; }
    const uint32_t flags = root->groupFlags(index);
    if ((flags & eGroupNoPortals) != 0) {
        if (root->loadedGroup(index) != nullptr) { visit(def, index); }
    } else if ((flags & eGroupExterior) != 0) {
        const walk::Inputs in = inputsFor(def);
        Env env{*this};
        walk::PortalWalk{root, in, env}.fromExterior(index, rect.remapped<double>(Rectf::basic(), Rectf::ndc()));
    }
}

void Pass::cullOutdoor(const Rectf& rect, float min_depth) {
    occluders_built_ = true;
    const occlusion::Build build{
        .horizon = horizon_,
        .camera_pos = camera_.camera_pos,
        .camera_target = camera_.camera_target,
        .min_depth = min_depth,
        .max_depth = SceneLists::maxDepth()
    };
    if (horizonOn()) { own_horizon_.reset(); }
    queue_.clear(scene_.last_row);
    const world_scene::FrustumScope scope{camera_.corners, rect};
    CWFrustum& top = world_scene::topFrustum();
    for (int32_t n = 0; n <= scene_.last_row; ++n) {
        cullChunks(top, n);
        cullOutdoorWmos(top, n, rect);
        cullOutdoorEntities(top, n);
        occlusion::accumulate(build, camera_.horizon_lanes,
            {.heights = own_horizon_.heights, .mask = own_horizon_.low_detail_mask}, queue_[n], scene_.edges[n],
            CMapChunk::kVertexOffsets);
    }
}

void Pass::cullChunks(CWFrustum& top, int32_t n) {
    if (n == SceneLists::kBucketCount - 1) { return; }
    const view::CameraView& v = camera_;
    for (CMapChunk* c : scene_.chunks[n]) {
        if (top.testBox(&c->bounds_) == 0 || map_low_detail::sphereOccluded(c->sphere_) || hidden(c->bounds_, 0) ||
            top.testBox(&c->detail_bounds_) == 0 || hidden(c->detail_bounds_, 0)) {
            continue;
        }
        const float depth = x87::viewDepth(v.view_plane, c->bounds_.nearCorner(v.camera_pos, v.camera_target));
        const occlusion::Chunk queued{c->origin_, depth, c->chunkHeights(), c->header_->holes_low_res};
        const int32_t k =
            queued.holes != 0 ? n : bucket::index(v.horizontal_view_plane, c->chunkCorner(v.quadrant_opposite));
        queue_.add(k, queued);
    }
}

void Pass::cullOutdoorWmos(CWFrustum& top, int32_t n, const Rectf& rect) {
    for (CMapObjDefGroup* g : scene_.groups[n]) {
        if (top.testBox(&g->bounds_) != 0 && !map_low_detail::sphereOccluded(g->sphere_) && !hidden(g->bounds_, 1)) {
            traverseGroupFromExterior(g, rect);
        }
    }
}

void Pass::cullOutdoorEntities(CWFrustum& top, int32_t n) const {
    for (CMapEntity* e : scene_.entities[n]) {
        const AaSphere& s = e->prev_sphere_;
        if (top.testSphere(&s) != 0 && !map_low_detail::sphereOccluded(s) &&
            occlusion::horizonSphere(horizon_, own_horizon_.heights, s.position, s.radius, 0) == 0) {
            markDrawn(e);
        }
    }
}

void Pass::cullInteriorGroups(bool exterior_view) {
    const world_scene::FrustumScope scope{camera_.corners, Rectf::basic()};
    for (CMapObjDefGroup* g : scene_.unbucketed) {
        const bool overlap = exterior_view || std::ranges::any_of(visits_.all(), [g](const Visits::Visit& v) {
            return v.group->bounds_.intersects(g->bounds_);
        });
        if (!overlap || world_scene::topFrustum().testBox(&g->bounds_) == 0) { continue; }
        traverseGroupFromExterior(g, Rectf::basic());
        cullPortalEntities(g, visits_.chainOf(g), true);
    }
}

void Pass::cullPortalGroups() {
    for (auto& [group, chain] : visits_.all()) {
        const CMapObjDef* def = group->def();
        if (def == world_scene::camera_map_obj_def ||
            (def->root_->groupFlags(group->group_index_) & kGroupOutdoor) == 0) {
            cullPortalEntities(group, chain, false);
        }
    }
}

void Pass::cullPortalEntities(const CMapObjDefGroup* g, std::span<CWFrustum> chain, bool all) const {
    for (CMapEntityLink* link : g->entities_) {
        CMapEntity* e = link->obj;
        if ((e->flags_ & CMapPlaceableObj::eNoSceneCull) != 0 ||
            (!all && (e->flags_ & CMapPlaceableObj::eInWmoGroup) == 0) || view_->drawn.contains(e) ||
            !scene_.wanted.contains(e)) {
            continue;
        }
        if (std::ranges::any_of(chain, [e](CWFrustum& f) { return f.testSphere(&e->prev_sphere_) != 0; })) {
            markDrawn(e);
        }
    }
}

using SetupHook = world_scene::setupCameraFrustum;
using SeamHook = world_scene::processCulledEntities;

class OpenViews {
public:
    Id open(const Spec& spec) {
        if (!spec.rect.isValid() || spec.side > Side::eDown || spec.wants == nullptr) { return 0; }
        const Id id = next_id_++;
        views_.push_back({.id = id, .spec = spec});
        update();
        return id;
    }

    void close(Id id) {
        std::erase_if(views_, [id](const View& v) { return v.id == id; });
        update();
    }

    bool reached(Id id, const CMapEntity* entity) const {
        const auto v = std::ranges::find(views_, id, &View::id);
        if (v == views_.end()) { return false; }
        const auto it = v->drawn.find(entity);
        return it != v->drawn.end() && it->second == entity->owner_guid_;
    }

    void apply(bool on) {
        on_ = on;
        if (!on) { forgetDrawn(); }
        update();
    }

    void leave() {
        forgetDrawn();
        frame_.reset();
    }

    void capture(const Vec3f& pos) {
        const CGxDevice* device = CGxDevice::getDevice();
        frame_ = Frame{.pos = pos, .view = device->xform(eGxXformView), .projection = device->projection_};
    }

    bool runFrame() {
        if (!frame_) { return false; }
        const bool turnable = frame_->turnable(world_scene::view_dir);
        if (turnable) {
            bool (*collected)(CMapEntity*) = nullptr;
            for (View& v : views_) {
                if (v.spec.wants != collected) {
                    collected = v.spec.wants;
                    collect(collected);
                }
                pass_.run(v, *frame_, wanted_);
            }
        } else {
            forgetDrawn();
        }
        frame_.reset();
        return turnable;
    }

    void keepAnimating() const {
        for (const View& v : views_) {
            if (!v.spec.animates) { continue; }
            for (const CMapEntity* e : v.drawn | std::views::keys) {
                CM2Model* model = e->model_;
                if (model != nullptr && (e->flags_ & CMapPlaceableObj::eInvisibleCarrier) == 0 &&
                    model->isLoaded(0, 0) != 0) {
                    model->setAnimating(1);
                }
            }
        }
    }

private:
    void update();

    void collect(bool (*wants)(CMapEntity*)) {
        wanted_.clear();
        for (CMapEntity* e : world_scene::map_entities) {
            if ((e->flags_ & CMapPlaceableObj::eNoSceneCull) == 0 && wants(e)) { wanted_.push_back(e); }
        }
    }

    void forgetDrawn() {
        for (View& v : views_) {
            v.drawn.clear();
        }
    }

    std::vector<View> views_;
    std::vector<CMapEntity*> wanted_;
    Id next_id_ = 1;
    bool on_ = false;
    std::optional<Frame> frame_;
    Pass pass_;
};

constexpr utils::Accessor<OpenViews, struct OpenViewsTag> kOpenViews;

void setupDetour(const Vec3f* pos, const Vec3f* target) {
    const Vec3f p = *pos;
    SetupHook{}(pos, target);
    kOpenViews->capture(p);
}

void seamDetour() {
    const bool ran = kOpenViews->runFrame();
    SeamHook{}();
    if (ran) { kOpenViews->keepAnimating(); }
}

void OpenViews::update() {
    const bool on = on_ && !views_.empty();
    if (on) {
        assertEnginePrecision();
    } else {
        frame_.reset();
    }
    static_cast<void>(hookkit::HookTransaction::reinstallIf<setupDetour>(SetupHook{}, on));
    static_cast<void>(hookkit::HookTransaction::reinstallIf<seamDetour>(SeamHook{}, on));
}
}  // namespace

Id open(const Spec& spec) { return kOpenViews->open(spec); }

void close(Id id) { kOpenViews->close(id); }

bool reached(Id id, const CMapEntity* entity) { return kOpenViews->reached(id, entity); }

void apply(bool on) { kOpenViews->apply(on); }

void initialize() {
    extensions::framescript::kOnLeave->add([] { kOpenViews->leave(); });
}
}  // namespace world_cull::views
