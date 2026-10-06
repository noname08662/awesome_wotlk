#pragma once

#include <cstdint>

#include "Map/CMapBaseObj.h"
#include "Math/Primitives.h"

class CM2Model;

class CMapPlaceableObj : public CMapBaseObj {
public:
    virtual char updateInteriorLight(int, int, uint16_t*, Vec3f*) = 0;  // 3

    enum Flags : uint32_t {
        eGroupClamped = 0x20,
        eBelowLightThreshold = 0x40,
        eLightingResolved = 0x80,
        eHasInteriorLight = 0x1000,
    };

    uint8_t _unk24;
    uint8_t _unk25;
    uint16_t _unk26;
    uint8_t _unk28;
    uint8_t _unk29;
    uint8_t _unk2A;
    uint8_t _unk2B;
    uint32_t trace_tag_;
    float light_metric_;
    CM2Model* model_;
    AaSphere prev_sphere_;
    AaBox prev_bbox_;
    Vec3f prev_sphere_center_;
    Vec3f position_;
    float scale_;
    Flags flags_;
    float resolved_height_;
    ColorBGRA<> ambient_col_;
    ColorBGRA<> diffuse_col_;
    float diffuse_blend_;
};

static_assert(sizeof(CMapPlaceableObj) == 0x90);
