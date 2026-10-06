#pragma once

#include <cstdint>

#include "Map/CMapPlaceableObj.h"

class CGUnit_C;
class CM2Model;

class CMapEntity : CMapPlaceableObj {
public:
    int(__cdecl* transform_callback_)(int, int, guid_t);
    float field_094;
    guid_t guid_;
    CGUnit_C* owner_unit_;
    int32_t field_0a4;
    float field_0a8;
    float field_0ac;
    float field_0b0;
    float field_0b4;
    int32_t ground_effect_id_;
    uint16_t wmo_group_id_;
    int16_t field_0be;
    ColorBGRA<> ambient_color_;
    float field_0c4;
    TSLink<CMapEntity> link_;

    HOOKKIT_HOOK_HANDLE(isValid, 0x0077F0B0, hookkit::Conv::eCdecl, int, CMapEntity*);
};

static_assert(sizeof(CMapEntity) == 0xD0);
