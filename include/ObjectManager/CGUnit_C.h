#pragma once

#include <cstdint>

#include "BaseTypes.h"

#include "Character/CCharacterComponent.h"
#include "DB/DBRecords.h"
#include "FX/CEffect.h"
#include "FX/CMissile.h"
#include "Movement/CMovement.h"
#include "ObjectManager/CGObject_C.h"
#include "ObjectManager/ObjectManager.h"
#include "ObjectManager/ObjectManagerEnums.h"

class CGWorldFrame;
class CGNamePlateFrame;

class CGUnit_C : public CGObject_C {
public:
    inline static auto& friendly_reach_added_dist = *reinterpret_cast<float*>(0x009E8D2C);
    inline static auto& default_interact_dist = *reinterpret_cast<float*>(0x00AC8A68);

    struct BattlegroundData {
        uint32_t instance_id;
        uint32_t map_id;
        unk_t unk_08;
        unk_t unk_0C;
        unk_t unk_10;
        unk_t unk_14;
        unk_t unk_18;
        unk_t unk_1C;
        uint32_t team_id;
    };

    virtual void nullsub66(int);                          // 66
    virtual void nullsub67(int);                          // 67
    virtual void nullsub68(int);                          // 68
    virtual void nullsub69(int);                          // 69
    virtual void* playUnitSound(int sound_id, int);       // 70
    virtual void* playFoleySound();                       // 71
    virtual int virtFC();                                 // 72
    virtual int stubReturnZero();                         // 73
    virtual bool isUnitFlagSet();                         // 74
    virtual void* getVirtualItem(int slot, char);         // 75
    virtual int getItemDisplayInfoRecord(int, uint8_t*);  // 76
    virtual int getVirtualItemDisplayId(int slot);        // 77
    virtual int updateVisualAttachment();                 // 78
    virtual int getSpellRank(int spell_id);               // 79
    virtual void* getDefenseSkillRank(void*, void*);      // 80
    virtual void* getAttackSkillRank(int, void*, void*);  // 81
    virtual int combatCalcHelper(int);                    // 82
    virtual double virt128();                             // 83

    enum SpellVisualKitEffectType : uint32_t {
        eEffectModelAttachment = 0x0,
        eEffectSound = 0x1,
        eEffectCameraShake = 0x2,
        eEffectLightingTint = 0x3,
        eEffectWorldEffect = 0x4,
        eEffectModelFlash = 0x5,
        eEffectFade = 0x6,
        eEffectAlphaFade = 0x7,
        eEffectHostileVisual = 0x8,
        eEffectWeaponTrail = 0x9,
        eEffectNone = 0xA,
        eNumEffects = 0xA,
    };

    struct VirtualItemPackedAttributes {
        uint8_t class_id;
        uint8_t subclass_id;
        uint8_t sound_override_subclass_id;
        uint8_t material;
        uint8_t inventory_type;
        uint8_t unk0;
        uint8_t sheathe_type;
        uint8_t unk1;
    };

    struct UnitThreat {
        TSLink<UnitThreat> link;
        TSHashObject<UnitThreat> hash_obj;
        guid_t guid;
    };

    static_assert(sizeof(UnitThreat) == 0x28);

    struct AuraData {
        unk_t pad[6];
    };

    static_assert(sizeof(AuraData) == 0x18);

    union UnitAuraData {
        AuraData auras_inline[16];
        TSGrowableArray<AuraData> auras;
    };

    struct SortedAuraInfo {
        uint32_t aura_index;
        uint32_t spell_id;
    };

    static_assert(sizeof(SortedAuraInfo) == 0x8);

    union UnitAurasSorted {
        SortedAuraInfo auras_sorted_inline[16];
        TSGrowableArray<SortedAuraInfo> auras_sorted;
    };

    union UnitSpellIds {
        uint32_t ids_inline[16];
        TSGrowableArray<uint32_t> ids;
    };

    union UnitSpellVisuals {
        SpellVisualKitEffectType visuals_inline[12];
        TSGrowableArray<SpellVisualKitEffectType> visuals;
    };

    struct DelayedSpellVisualKit {
        unk_t pad[12];
    };

    UnitEntry* descriptors_;
    unk_t _unk_D4;
    CUnitMovement* movement_ptr_;
    TSExplicitList<void> lists_[142];
    uint32_t _lists_end;
    CUnitMovement movement_;
    unk_t _movement_and_sound[37];
    CreatureCache* creature_cache_;
    CreatureDisplayInfoRec* creature_display_info_rec_;
    CreatureDisplayInfoExtraRec* creature_display_info_extra_rec_;
    CreatureModelDataRec* creature_model_data_rec_;
    CreatureSoundDataRec* creature_sound_data_rec_;
    CreatureSoundDataRec* _unused_sound_rec;
    UnitBloodLevelsRec* unit_blood_levels_rec_;
    CEffect* effect_;
    CEffect* effect2_;
    CMissile* missle_;
    unk_t _unk_980[3];
    uint32_t virtual_weap_display_info_id_[3];
    VirtualItemPackedAttributes virtual_weap_attributes_[3];
    unk_t _unk_9BC[29];
    uint32_t creature_flags_;
    unk_t _unk_A34;
    uint32_t model_flags_;
    uint32_t _unk_A3C[8];
    int8_t spell_flag_;
    char _pad[3];
    uint32_t spell_row_id_;
    uint32_t _unk_A64[2];
    uint32_t cast_spell_id_;
    guid_t cast_target_guid_;
    uint32_t cast_start_time_;
    uint32_t cast_end_time_;
    uint32_t channel_spell_id_;
    uint32_t channel_start_time_;
    uint32_t channel_end_time_;
    uint32_t _unk_A8C;
    uint32_t _unk_A90[44];
    CM2Model* _model_B4C;
    CM2Model* _model_B44;
    unk_t _unk_B48;
    CCharacterComponent* comp_;
    unk_t _unk_B50[58];
    CGNamePlateFrame* nameplate_;
    CSimpleFrame* chat_bubble_;
    unk_t _unk_C40[4];
    UnitAuraData auras_;
    uint32_t auras_count_;
    UnitAurasSorted buffs_;
    uint32_t buffs_count_;
    UnitAurasSorted debuffs_;
    uint32_t debuffs_count_;
    UnitSpellIds ids_;
    uint32_t ids_count_;
    UnitSpellVisuals visuals_;
    DelayedSpellVisualKit* delayed_visuals_;
    uint32_t delayed_visuals_count_;
    uint32_t visual_fx_token_;
    void* vehicle_;
    void* passenger_;
    void* vehicle_cam_;
    unk_t _unk_F68[16];
    CM2Model* _unk_model;
    unk_t _unk_FAC[13];
    TSHashTable<UnitThreat> threat_tsht_;

    HOOKKIT_HOOK(getShapeshiftFormId, 0x0071AF70, hookkit::Conv::eThiscall, ShapeshiftForm, CGUnit_C*);
    HOOKKIT_HOOK(getCreatureRank, 0x00718A00, hookkit::Conv::eThiscall, CreatureRank, CGUnit_C*);
    HOOKKIT_HOOK(getUnitReaction, 0x007251C0, hookkit::Conv::eThiscall, UnitReaction, CGUnit_C*, CGUnit_C*);
    HOOKKIT_HOOK(canAssist, 0x007293D0, hookkit::Conv::eThiscall, bool, CGUnit_C*, CGUnit_C*, char);
    HOOKKIT_HOOK(canAttack, 0x00729A70, hookkit::Conv::eThiscall, bool, CGUnit_C*, CGUnit_C*);
    // enum all visible units
    HOOKKIT_HOOK(updateReaction, 0x0071F8F0, hookkit::Conv::eThiscall, int, CGUnit_C*, char);
    HOOKKIT_HOOK(hideNamePlate, 0x00725840, hookkit::Conv::eThiscall, CGNamePlateFrame*, CGUnit_C*);
    HOOKKIT_HOOK(setNamePlateFocus, 0x007271D0, hookkit::Conv::eCdecl, void, Vec3f*);
    HOOKKIT_HOOK(getName, 0x0072A000, hookkit::Conv::eThiscall, const char*, CGUnit_C*, void*, char);
    HOOKKIT_HOOK(getMissleTargetPos, 0x0071A720, hookkit::Conv::eThiscall, int, CGUnit_C*, Vec3f*);
    HOOKKIT_HOOK(isVisible, 0x00715720, hookkit::Conv::eThiscall, bool, CGUnit_C*, CGWorldFrame*, Vec3f*);

    bool isFriendly(CGUnit_C* player = object_mgr::get<CGUnit_C>(object_mgr::getPlayerGuid(), eTypemaskPlayer)) const {
        if (!player) { return false; }
        UnitReaction reaction = player->getUnitReaction(const_cast<CGUnit_C*>(this));
        return (reaction >= eReactionFriendly) ||
            (reaction == eReactionNeutral && !player->canAttack(const_cast<CGUnit_C*>(this)));
    }

    float getInteractDistanceSq() const {
        float reach = descriptors_->combatreach;
        float val = reach + friendly_reach_added_dist;  // 4.0f
        return val * val;
    }
};
