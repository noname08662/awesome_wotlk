#pragma once

#include "BaseTypes.h"

#include "M2/CM2Light.h"
#include "M2/CM2Lighting.h"
#include "M2/CRibbonEmitter.h"
#include "M2/M2Data.h"
#include "M2/M2Sequence.h"
#include "Map/CMapDoodadDef.h"

class CM2Shared;
class CM2Scene;
class CGCamera;
class CShaderEffect;

struct CParticleSystem2;

class CM2Model {
public:
    enum Flags04 : uint32_t {
        eRecomputeCameraDist = 0x1,
        ePendingInitializeLoaded = 0x20,
    };

    enum Flags10 : uint32_t {
        eLoaded = 0x1,
        eTexturesReadyCached = 0x2,
        eDrawThisFrame = 0x8,
        eDoodadBatchCompatible = 0x10,
        eUnkBit06 = 0x40,
        eDrawWithParent = 0x80,
        eAllChildrenLoadedCached = 0x100,
        eAllChildrenDrawableCached = 0x200,
        eHasActiveParticles = 0x400,
        eFlushingDeferredCalls = 0x800,
        eUseSimpleAnimate = 0x1000,
        eWorldTransformSet = 0x8000,
        eParticlesThisFrame = 0x10000,
        eParticlesWithParent = 0x20000,
        eAttachedWithLocalOffset = 0x40000,
        eUnkBit19 = 0x80000,
        eUnkBit20 = 0x100000,
        eUnkBit21 = 0x200000,
        eHasActiveSequenceCallback = 0x400000,
    };

    struct CallNode {
        enum CallType {
            eHandle = 0x0,
            eSetRibbonsEnabled = 0xD,
            eSetGeometryVisible = 0x1,
            eUnk2 = 0x2,
            eOptimizeVisibleGeometry = 0x3,
            eUnk4 = 0x4,
            eSetBoneSequence = 0x5,
            eUnsetBoneSequence = 0x6,
            eUnk7 = 0x7,
            eUnk8 = 0x8,
            eUnk9 = 0x9,
            eUnk10 = 0xA,
            eUnk11 = 0xB,
            eSetEmittersEnabled = 0xC,
            eUnk14 = 0xE,
        };

        uint32_t type;
        CallNode* call_next;
        uint32_t time;
        int enabled;
        uint32_t args[16];
    };

    static_assert(sizeof(CallNode) == 0x50);

    uint32_t ref_count_;
    Flags04 flags04_;
    SlotLink<CM2Model> scene_link_;
    Flags10 flags10_;
    uint16_t primary_bone_link_head_;
    char _pad16[2];
    SlotLink<CM2Model> callback_link_;
    void(__stdcall* loaded_callback_)(CM2Model* model, void* arg);
    void* loaded_callback_arg_;
    CM2Scene* scene_;
    CM2Shared* shared_;
    CM2Model* duplicate_source_;
    SlotList<CallNode> call_list_;
    uint32_t last_anim_tick_;
    SlotLink<CM2Model> animate_link_;
    CM2Model* attach_parent_;
    M2Track<uint8_t>* visibility_track_;  // trailing char pad[3]
    uint32_t attach_point_id_;
    uint32_t attach_bone_index_;
    CM2Model* first_child_;
    SlotLink<CM2Model> sibling_link_;
    uint32_t last_update_time_;
    SlotLink<CM2Model> draw_link_;
    uint32_t* global_sequence_times_;
    uint32_t spawn_time_;
    void(__cdecl* sequence_callback_)(CM2Model* model, uint32_t bone_id, uint32_t bone_callback_arg, int is_immediate,
        uint32_t elapsed_since_boundary, uint64_t context);
    void* _alignment7C;
    uint64_t sequence_callback_arg_;
    float camera_dist_sq_;
    uint32_t init_time_;
    uint32_t skip_static_tracks_;
    M2ModelBone* bones_;
    Mat4f* bone_matrices_;
    uint32_t* skin_section_visible_;
    M2ModelColor* colors_;
    CTexture* textures_;
    M2TextureWeight* texture_weights_;
    M2ModelTextureTransform* texture_transform_;
    Mat4f* texture_matricies_;
    Mat4f world_transform_;
    Mat4f world_view_transform_;

    Mat4f _unused_matrix;
    uint32_t _unused_attachment_copy;

    float base_alpha_;
    float fade_alpha_;
    Vec3f diffuse_color_;
    Vec3f emissive_color_;
    float propagated_alpha_;
    float effective_alpha_;
    Vec3f effective_diffuse_color_;
    Vec3f effective_emissive_color_;
    float _unused_emissive_alpha;
    // typedef int(__usercall* CM2ModelDrawCallback) @<eax>(int a1 @<ebx>, int a2 @<edi>, int a3 @<esi>, int a4, float* a5);
    void* draw_callback_;
    void* draw_callback_arg_;
    void(__cdecl* event_callback_)(CM2Model* model, uint32_t bone_id, uint32_t bone_callback_arg, int is_immediate,
        const Vec3f* position, uint32_t elapsed_since_boundary, uint64_t context);
    uint64_t event_callback_arg_;
    M2ModelLight* light_;
    CM2Lighting self_lighting_;
    CM2Lighting* effective_lightning_;
    void(__cdecl* lightning_override_callback_)(CM2Model* model, CM2Lighting* lighting, void* arg);
    void* lightning_override_arg_;
    M2ModelCamera* cameras_;
    M2ModelRibbon* ribbon_states_;
    CRibbonEmitter* ribbon_emitters_;
    M2ModelParticle* particle_states_;
    CParticleSystem2** particle_systems_;
    SlotLink<CM2Model> particle_link_;
    M2ModelOptGeo* optimized_geometry_;
    M2ModelRaycastMode raycast_mode_;
    SlotLink<CM2Model> raycast_link_;
    CMapDoodadDef* raycast_doodad_def_;
    uint32_t raycast_hit_tag_;
    uint32_t heap_alloc_id_;
    float _out_pad;

    HOOKKIT_HOOK(ctor, 0x0082BE60, hookkit::Conv::eThiscall, CM2Model*, CM2Model*);
    HOOKKIT_HOOK(dtor, 0x00832640, hookkit::Conv::eThiscall, void, CM2Model*);
    HOOKKIT_HOOK(setAnimating, 0x00823F10, hookkit::Conv::eThiscall, void, CM2Model*, int);
};

static_assert(sizeof(CM2Model) == 0x2F0);
