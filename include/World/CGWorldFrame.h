#pragma once

#include <cstddef>
#include <cstdint>

#include "BaseTypes.h"

#include "UI/CGNamePlateFrame.h"
#include "Widget/CSimpleCamera.h"
#include "Widget/CSimpleFrame.h"

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

    struct TerrainClickEvent {
        guid_t guid;
        Vec3f pos;
        uint32_t button;
    };

    static constexpr uint32_t kWorldTraceHitFlags = 0x100111;

    TSExplicitList<void> _unk_list1;
    TSExplicitList<void> _unk_list2;
    TSExplicitList<void> _unk_list3;

    uint32_t render_state_;
    guid_t layer_track_;
    unk_t _unk_2C4[2];
    uint32_t pending_action_id_;
    int32_t default_action_result_;
    float _unk_2E0[2];

    uint32_t roll_transition_start_ms_;
    uint32_t roll_transition_end_ms_;
    float roll_from_;
    float roll_to_;
    float camera_roll_;
    unk_t _unk_2FC[5];

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

    HOOKKIT_HOOK_HANDLE(handleTerrainClick, 0x00527830, hookkit::Conv::eCdecl, void, TerrainClickEvent*);

    HOOKKIT_HOOK_HANDLE(intersect, 0x0077F310, hookkit::Conv::eCdecl, char, Vec3f*, Vec3f*, Vec3f*, float*, uint32_t,
        WorldIntersectHitBuffer*);

    HOOKKIT_HOOK_HANDLE(updateNamePlatePositions, 0x00725890, hookkit::Conv::eCdecl, void, CGWorldFrame*);
    HOOKKIT_HOOK_HANDLE(updateNamePlatePosition, 0x00615E10, hookkit::Conv::eCdecl, char, CGNamePlateFrame*,
        CGWorldFrame*, Vec3f*, int);

    HOOKKIT_HOOK_HANDLE(getDoodadHitDetails, 0x007A2C60, hookkit::Conv::eCdecl, void, WorldIntersectHitBuffer*, void*);

    HOOKKIT_HOOK(getScreenCoords, 0x004F6D20, hookkit::Conv::eThiscall, bool, CGWorldFrame*, Vec3f*, Vec3f*, int*);
    HOOKKIT_HOOK(onLayerTrackTerrain, 0x004F66C0, hookkit::Conv::eThiscall, int, CGWorldFrame*, TerrainClickEvent*);

    HOOKKIT_HOOK_HANDLE(handleNameplateLeftClick, 0x005274F0, hookkit::Conv::eCdecl, int, guid_t);
    HOOKKIT_HOOK_HANDLE(handleNameplateRightClick, 0x005277B0, hookkit::Conv::eCdecl, int, guid_t);

    static void percToScreenPos(float x, float y, float* res_x, float* res_y) {
        if (!res_x || !res_y) { return; }
        float screen_height_aptitude = *reinterpret_cast<float*>(0x00AC0CBC);
        float some_val = *reinterpret_cast<float*>(0x00AC0CB4);
        if (std::abs(some_val) < 1e-6f) { return; }
        float scale = (screen_height_aptitude * 1024.0f) / some_val;
        *res_x = x * scale;
        *res_y = y * scale;
    }
};

static_assert(offsetof(CGWorldFrame, roll_transition_start_ms_) == 0x2E8);
static_assert(offsetof(CGWorldFrame, view_bottom_) == 0x330);
static_assert(offsetof(CGWorldFrame, render_dirty_flags_) == 0xB10);
static_assert(offsetof(CGWorldFrame, scene_time_) == 0xB14);
static_assert(offsetof(CGWorldFrame, camera_) == 0x7E20);
