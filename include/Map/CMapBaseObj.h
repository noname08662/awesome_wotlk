#pragma once

#include <cstdint>

#include "Lib/Storm.h"

class CM2Lighting;

class CMapBaseObj {
public:
    virtual ~CMapBaseObj();                           // 0
    virtual CM2Lighting* selectLights(CM2Lighting*);  // 1
    virtual int resolveWmoGroup(int);                 // 2

    enum Flags : uint32_t {
        ePendingReset = 0x1,
        eCustomLight = 0x2,
        eBiodome = 0x800,
        eAltDayNight = 0x8000,
    };

    enum KindFlags : uint16_t {
        eKindWmoGroup = 0x10,
        eKindM2DoodadInWmo = 0x20,
        eKindM2Doodad = 0x40,
    };

    uint32_t out_ptr_;
    KindFlags flags_;
    uint16_t _unk0A;
    Flags light_flags_;
    TSLink<void> registry_link_;
    TSExplicitList<void> list_;
};

static_assert(sizeof(CMapBaseObj) == 0x24);
