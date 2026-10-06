#pragma once

#include "M2/CM2Cache.h"
#include "M2/CM2Light.h"
#include "M2/M2Element.h"
#include "Map/CMapDoodadDef.h"

class CM2Scene {
public:
    struct HitRecord {
        CM2Model* model;
        float distance_near;
        float distance_far;
        uint32_t flags;
    };

    enum Flags {
        eRaycasting = 0x2,
        eAdvancingTime = 0x4,
    };

    uint32_t ref_count_;
    CM2Cache* cache_;
    CM2Model* all_models_list_;
    uint32_t time_;
    uint32_t last_elapsed_;
    uint32_t anim_tick_;
    float _unused_float18;
    Flags flags_;
    SlotList<CM2Light> light_list_;
    CM2Model* animate_list_;
    CM2Model* draw_list_;
    CM2Model* particle_animate_list_;
    TSGrowableArray<M2Element> elements_;
    TSGrowableArray<uint32_t> element_indices_extra_;
    TSGrowableArray<uint32_t> element_indices_by_pass_[3];
    Mat4f view_;
    Mat4f view_inv_;
    void(__cdecl* draw_batch_proj_callback_)(
        Vec3f* unused_screen_pos, Vec4u8* color, int flag30, int unused_userdata, int is_unlit);
    int _unused_draw_batch_proj_userdata;
    int(__cdecl* raycast_callback_)(Vec3f* start, float* hit_distance, int unused_userdata);
    int _unused_raycast_callback_userdata;
    CM2Model* raycast_list_;
    HitRecord* hit_records_;
    uint32_t* hit_record_order_;
    uint32_t hit_records_capacity_;
    Vec3f* vertex_cache_;
    uint32_t vertex_cache_capacity_;
    HitRecord last_hit_;
    CMapDoodadDef* last_hit_doodad_def_;
    uint32_t camera_liquid_type_;
    uint32_t m2_pass_mask_;

    static CM2Scene* get() { return *reinterpret_cast<CM2Scene**>(0x00CD754C); }

    HOOKKIT_HOOK(ctor, 0x008216C0, hookkit::Conv::eThiscall, CM2Scene*, CM2Scene*, CM2Cache*);
    HOOKKIT_HOOK(dtor, 0x00821850, hookkit::Conv::eThiscall, void, CM2Scene*);

    // start, end, hit distance fraction, allow retry
    HOOKKIT_HOOK(
        rayCastModels, 0x0081DF10, hookkit::Conv::eThiscall, CMapDoodadDef*, CM2Scene*, Vec3f*, Vec3f*, float*, int);
};

static_assert(sizeof(CM2Scene) == 0x148);
