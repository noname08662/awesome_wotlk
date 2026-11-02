#pragma once

#include <cstdint>

#include "M2/CM2Model.h"
#include "Map/CMapPlaceableObj.h"

struct BlizzardObject;
class CGUnit_C;
class CMapObjDefGroup;
class CMapObjDef;

struct EntityRayHit {
    CMapObjDef* def;
    CMapObjDefGroup* def_group;
    float t;
    uint16_t _unk0C;
    uint16_t interior;
};

static_assert(sizeof(EntityRayHit) == 0x10);

class CMapEntity : public CMapPlaceableObj {
public:
    int(__cdecl* transform_callback_)(BlizzardObject*, int, guid_t, CGUnit_C*);
    BlizzardObject* blizz_obj_;
    guid_t owner_guid_;
    CGUnit_C* missile_unit_;
    unk_t _unk_A4[5];
    int32_t ground_effect_id_;
    uint16_t wmo_group_id_;
    int16_t _unk_BE;
    ColorBGRA<> ambient_color_;
    float _unk_C4;
    TSLink<CMapEntity> link_;

    bool isBareCarrier() const {
        return (flags_ & eInvisibleCarrier) != 0 && (model_ == nullptr || model_->first_child_ == nullptr);
    }

    // bucketed to culled_entities_, where only the portal passes reach it
    bool portalOnly() const { return (flags_ & eInWmoGroup) != 0 || isBareCarrier(); }

    HOOKKIT_HOOK_HANDLE(isDrawn, 0x0077F0B0, hookkit::Conv::eCdecl, int, CMapEntity*);
    HOOKKIT_HOOK_HANDLE(castGroundRay, 0x007C28F0, hookkit::Conv::eCdecl, void, CMapEntity*, Vec3f*, Vec3f*, Vec3f*,
        uint32_t*, uint32_t*, EntityRayHit*, EntityRayHit*);
};

static_assert(sizeof(CMapEntity) == 0xD0);
