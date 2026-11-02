#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "BaseTypes.h"
#include "Constants.h"

#include "Math/Math.h"
#include "ObjectManager/CGUnit_C.h"
#include "ObjectManager/ObjectManager.h"
#include "UI/CGNamePlateFrame.h"
#include "Widget/CSimpleCamera.h"
#include "Widget/CSimpleFrame.h"
#include "World/PlayerNames.h"

class CM2Model;

struct WorldIntersectModelHit {
    const char* model_path;
    uint32_t child_depth;
    AaBox bounds;
    AaSphere hit_sphere;
    uint32_t visible_batch_count;
    uint32_t visible_triangle_count;
    uint32_t effect_primitive_count;
    uint32_t bone_count;
    uint32_t light_count;
    uint32_t active_light_count;
    uint32_t attach_point_id;
    float attach_scale;
    CM2Model* model;
    uint32_t doodad_compatible_batch_count;
    uint32_t max_batch_instances;
    float hit_param0;
    float hit_param1;
    uint32_t hit_param2;
    uint32_t _pad68;
};

static_assert(sizeof(WorldIntersectModelHit) == 0x6C);

struct WorldIntersectSurfaceMaterial {
    const char* name;
    uint32_t tag;
    float unk8;
    float unk12;
};

static_assert(sizeof(WorldIntersectSurfaceMaterial) == 0x10);

struct WorldIntersectWmoHitInfo {
    float fields[13];
};

static_assert(sizeof(WorldIntersectWmoHitInfo) == 0x34);

struct WorldIntersectHitBuffer {
    uint32_t type;
    uint32_t model_hit_count;
    uint32_t surface_material_count;
    WorldIntersectModelHit model_hits[16];
    WorldIntersectSurfaceMaterial surface_materials[8];
    WorldIntersectWmoHitInfo wmo_hit;
};

static_assert(sizeof(WorldIntersectHitBuffer) == 0x780);

class CGWorldFrame : public CSimpleFrame {
public:
    struct CGWorldFrameUnk {
        unk_t _unk_00[1094];
        float mouse_x;
        float mouse_y;
    };

    // not CGInputControl::ClickType
    enum ClickButton : int {
        eClickButtonLeft = 1,
        eClickButtonRight = 4,
    };

    struct TerrainClickEvent {
        guid_t guid;
        Vec3f pos;
        ClickButton button;
    };

    enum HitTestKind : int {
        eHitNothing,
        eHitTerrain,
        eHitModel,
        eHitWorldObject,
    };

    // pos is object local when guid is a transport
    struct HitTestResult {
        guid_t guid;
        Vec3f pos;
        float hit_dist;
        Vec3f ray_start;
        Vec3f ray_end;

        [[nodiscard]]
        Vec3f worldPos() const {
            auto* object = guid != 0 ? object_mgr::get<CGObject_C>(guid, eTypemaskObject) : nullptr;
            if (object == nullptr || !object->isTransport()) { return pos; }
            Mat4f matrix;
            return math::transformPoint(pos, *object->getMatrix(&matrix));
        }

        void setWorldPos(const Vec3f& world, guid_t object_guid) {
            guid = 0;
            pos = world;
            if (!object_mgr::isTransportGuid(object_guid)) { return; }
            if (auto* object = object_mgr::get<CGObject_C>(object_guid, eTypemaskObject)) {
                Mat4f matrix;
                pos = math::transformPoint(world, math::affineInverse(*object->getMatrix(&matrix)));
                guid = object_guid;
            }
        }
    };

    static_assert(sizeof(HitTestResult) == 0x30);

    enum HitFlags : uint32_t {
        eHitTestBoundingModels = 0x00000001,
        eHitTestDoodadMesh2 = 0x00000002,
        eHitTestDoodadMesh4 = 0x00000004,
        eHitTestDoodadMesh8 = 0x00000008,
        eHitTestWMO = 0x00000010,
        eHitTestWmoPolys20 = 0x00000020,
        eHitTestUnknown = 0x00000040,
        eHitTestGround = 0x00000100,
        eHitTestLiquid = 0x00010000,
        eHitTestAllLiquids = 0x00020000,
        eHitTestMovableObjects = 0x00100000,
        eHitTestDrawnOnly = 0x01000000,
    };

    static constexpr uint32_t kWorldTraceHitFlags =
        eHitTestBoundingModels | eHitTestWMO | eHitTestGround | eHitTestMovableObjects;
    static_assert(kWorldTraceHitFlags == 0x100111);

    // HitTestWorldMouseOver's; terrain is its first trace when terrain is targetable
    static constexpr uint32_t kCursorTraceFlags =
        eHitTestDoodadMesh4 | eHitTestWmoPolys20 | eHitTestGround | eHitTestDrawnOnly;
    static constexpr uint32_t kCursorTerrainFlags = kCursorTraceFlags | eHitTestAllLiquids;
    static constexpr uint32_t kCursorCollisionFlags = kWorldTraceHitFlags | eHitTestUnknown;
    static constexpr uint32_t kDoodadHitFlags =
        eHitTestBoundingModels | eHitTestDoodadMesh2 | eHitTestDoodadMesh4 | eHitTestDoodadMesh8;
    static_assert(kCursorTraceFlags == 0x1000124);
    static_assert(kCursorTerrainFlags == 0x1020124);
    static_assert(kCursorCollisionFlags == 0x100151);
    static_assert(kDoodadHitFlags == 0xF);

    TSExplicitList<void> _unk_list1;
    TSExplicitList<void> _unk_list2;
    TSExplicitList<void> _unk_list3;

    uint32_t render_state_;
    guid_t layer_track_;
    unk_t _unk_2C4[2];
    HitTestKind pending_action_id_;
    int32_t default_action_result_;
    HitTestResult default_action_hit_;

    float screen_x_ndc_to_ddc_;
    float screen_y_ndc_to_ddc_;

    unk_t unknown_flags_;
    uint32_t flags_31C_;
    unk_t _unk_320[4];

    float view_bottom_;
    float view_left_;
    float view_top_;
    float view_right_;
    Mat4f view_matrix_;

    CGWorldFrameUnk* data_;
    uint32_t light_system_[479];
    unk_t _unk_B00;
    float last_update_time_;

    unk_t _unk_B08;
    unk_t _unk_B0C;
    uint32_t render_dirty_flags_;
    float scene_time_;
    uint32_t scene_object_cache_[7079];

    unk_t _unk_79B4[8];
    uint32_t data_block_unk_[275];

    CSimpleCamera* camera_;

    static CGWorldFrame* get() { return *reinterpret_cast<CGWorldFrame**>(0x00B7436C); }

    // the transport the last trace landed on
    inline static auto& last_trace_object = *reinterpret_cast<guid_t*>(0x00CD7768);

    HOOKKIT_HOOK_HANDLE(handleTerrainClick, 0x00527830, hookkit::Conv::eCdecl, void, TerrainClickEvent*);

    HOOKKIT_HOOK_HANDLE(intersect, 0x0077F310, hookkit::Conv::eCdecl, char, Vec3f*, Vec3f*, Vec3f*, float*, uint32_t,
        WorldIntersectHitBuffer*);

    HOOKKIT_HOOK_HANDLE(updateNamePlatePositions, 0x00725890, hookkit::Conv::eCdecl, void, CGWorldFrame*);
    HOOKKIT_HOOK_HANDLE(updateNamePlatePosition, 0x00615E10, hookkit::Conv::eCdecl, char, CGNamePlateFrame*,
        CGWorldFrame*, Vec3f*, int);

    HOOKKIT_HOOK_HANDLE(getDoodadHitDetails, 0x007A2C60, hookkit::Conv::eCdecl, void, WorldIntersectHitBuffer*, void*);

    HOOKKIT_HOOK(getScreenCoords, 0x004F6D20, hookkit::Conv::eThiscall, bool, CGWorldFrame*, Vec3f*, Vec3f*, int*);
    HOOKKIT_HOOK(onLayerTrackTerrain, 0x004F66C0, hookkit::Conv::eThiscall, int, CGWorldFrame*, TerrainClickEvent*);
    // x, y, filter, out
    HOOKKIT_HOOK(hitTestPoint, 0x004F9DA0, hookkit::Conv::eThiscall, HitTestKind, CGWorldFrame*, float, float, uint32_t,
        HitTestResult*);
    // acts on default_action_hit_
    HOOKKIT_HOOK(performDefaultAction, 0x004F7880, hookkit::Conv::eThiscall, int, CGWorldFrame*, ClickButton);

    HOOKKIT_HOOK_HANDLE(handleNameplateLeftClick, 0x005274F0, hookkit::Conv::eCdecl, int, guid_t);
    HOOKKIT_HOOK_HANDLE(handleNameplateRightClick, 0x005277B0, hookkit::Conv::eCdecl, int, guid_t);

    // HitTestWorldMouseOver's two traces: drawn, then collision up to its hit
    static bool pickWorld(
        Vec3f from, Vec3f to, uint32_t drawn_flags, uint32_t collision_flags, Vec3f& hit, guid_t& object) {
        float dist = 0.0f;
        const bool drawn = math::trace(from, to, drawn_flags, hit, dist);
        if (drawn) {
            object = last_trace_object;
            to = hit;
        }

        Vec3f nearer;
        if (!math::trace(from, to, collision_flags, nearer, dist)) { return drawn; }
        hit = nearer;
        object = last_trace_object;
        return true;
    }

    // only a transport's guid travels: pos is local to it and world for any other object
    static void leftClickTerrain(guid_t guid, const Vec3f& pos) {
        TerrainClickEvent click{
            .guid = object_mgr::isTransportGuid(guid) ? guid : 0, .pos = pos, .button = eClickButtonLeft};
        handleTerrainClick{}(&click);
    }

    static void percToScreenPos(float x, float y, float* res_x, float* res_y) {
        if (!res_x || !res_y) { return; }
        float screen_height_aptitude = *reinterpret_cast<float*>(0x00AC0CBC);
        float some_val = *reinterpret_cast<float*>(0x00AC0CB4);
        if (std::abs(some_val) < 1e-6f) { return; }
        float scale = (screen_height_aptitude * 1024.0f) / some_val;
        *res_x = x * scale;
        *res_y = y * scale;
    }

    struct Projection {
        int mask;  // 1 x >= 0, 2 y >= 0, 4 x <= w, 8 y <= h
        bool behind;
        float depth;  // clip w, the view depth
    };

    // 0x004F6D20 without its near plane early out
    Projection projectName(const Vec3f& world_pos, Vec3f& out) const {
        const Vec3f d = world_pos - camera_->pos_;
        const Mat4f& m = view_matrix_;
        const auto col = [&](int c) { return d.x * m[0][c] + d.y * m[1][c] + d.z * m[2][c]; };
        const Vec4f clip{col(0), col(1), col(2), col(3)};

        const float near_clip = camera_->near_clip_;
        const bool behind = clip.z < near_clip;
        Vec2f ndc;
        if (!behind) {
            ndc = Vec2f{clip.x, clip.y} * (1.0f / clip.w);
            out.z = clip.z;
        } else {
            // the front divide with the depth's sign dropped
            ndc = Vec2f{clip.x, clip.y} * (1.0f / std::max(std::abs(clip.w), near_clip));
            const float extent = std::max(std::abs(ndc.x), std::abs(ndc.y));
            if (extent < kEps4f) {
                ndc = {0.0f, -1.0f};  // straight behind, bottom edge
            } else if (extent < 1.0f) {
                ndc *= 1.0f / extent;  // lands on screen, out along its ray to the edge
            }
            out.z = std::max(std::abs(clip.z), near_clip);  // keeps the draw order sorted by distance
        }

        constexpr Vec2f kRayDepthOffY{12.0f, 3.0f};  // yd, starts and finishes
        constexpr Vec2f kRayDepthOffX{3.0f, 1.0f};
        const float extent = std::max(std::abs(ndc.x), std::abs(ndc.y));
        const bool off_y = std::abs(ndc.y) > std::abs(ndc.x);
        const Vec2f ray_depth = off_y ? kRayDepthOffY : kRayDepthOffX;
        if (extent > 1.0f && clip.w < ray_depth.x) {
            float& on_screen = off_y ? ndc.x : ndc.y;
            const float t = std::min((ray_depth.x - clip.w) / (ray_depth.x - ray_depth.y), 1.0f);
            on_screen = std::lerp(std::clamp(on_screen, -1.0f, 1.0f), on_screen / extent, t * t * (3.0f - 2.0f * t));
        }

        const Vec2f ddc = math::ndcToDdc(
            {(ndc.x + 1.0f) * 0.5f * (view_right_ - view_left_), (ndc.y + 1.0f) * 0.5f * (view_top_ - view_bottom_)});
        const Rectf& r = rect_;
        out.x = r.left < 0.0f ? ddc.x - r.left : ddc.x;
        out.y = r.bottom < 0.0f ? ddc.y - r.bottom : ddc.y;
        const int mask = (out.x >= 0.0f ? 1 : 0) | (out.y >= 0.0f ? 2 : 0) | (out.x <= r.right - r.left ? 4 : 0) |
            (out.y <= r.top - r.bottom ? 8 : 0);
        return {mask, behind, clip.w};
    }

    Vec2f nameHalfExtent(CGUnit_C* unit, float depth, float offset) const {
        constexpr float kLineHeight = 0.2f;    // yd
        constexpr Vec2f kHalfEms{8.0f, 5.0f};  // a long name; a guild line and the raid icon above it
        const float em = kLineHeight * CGObject_C::getNameFontScale{}(unit);
        const Mat4f& m = view_matrix_;
        const auto scale = [&](int c) { return std::sqrt(m[0][c] * m[0][c] + m[1][c] * m[1][c] + m[2][c] * m[2][c]); };
        return {kHalfEms.x * em * scale(0) / depth * 0.5f * (view_right_ - view_left_),
            (kHalfEms.y * em + std::abs(offset)) * scale(1) / depth * 0.5f * (view_top_ - view_bottom_)};
    }
};

static_assert(offsetof(CGWorldFrame, pending_action_id_) == 0x2D8);
static_assert(offsetof(CGWorldFrame, default_action_hit_) == 0x2E0);
static_assert(offsetof(CGWorldFrame, view_bottom_) == 0x330);
static_assert(offsetof(CGWorldFrame, render_dirty_flags_) == 0xB10);
static_assert(offsetof(CGWorldFrame, scene_time_) == 0xB14);
static_assert(offsetof(CGWorldFrame, camera_) == 0x7E20);
