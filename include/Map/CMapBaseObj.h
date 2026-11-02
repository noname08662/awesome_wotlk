#pragma once

#include <cstdint>

#include "Lib/Storm.h"

class CM2Lighting;
class CMapBaseObj;

template <typename Obj = CMapBaseObj, typename Owner = CMapBaseObj>
struct CMapBaseObjLink {
    uint32_t out_ptr;
    Obj* obj;
    Owner* owner;
    TSLink<CMapBaseObjLink> owner_link;
    TSLink<CMapBaseObjLink> obj_link;
};

static_assert(sizeof(CMapBaseObjLink<>) == 0x1C);

class CMapEntity;
class CMapObjDef;
class CMapObjDefGroup;

using CMapEntityLink = CMapBaseObjLink<CMapEntity, CMapObjDefGroup>;
using CMapObjDefGroupLink = CMapBaseObjLink<CMapObjDefGroup, CMapObjDef>;

class CMapBaseObj {
public:
    virtual ~CMapBaseObj();                           // 0
    virtual CM2Lighting* selectLights(CM2Lighting*);  // 1
    virtual int resolveWmoGroup(int);                 // 2

    enum Flags : uint32_t {
        ePendingReset = 0x1,
        eInteriorLit = 0x2,
        eExteriorLit = 0x4,
        eBiodome = 0x800,
        eInteriorPath = 0x8000,
    };

    enum KindFlags : uint16_t {
        eKindChunk = 0x4,
        eKindWmoGroup = 0x10,
        eKindM2DoodadInWmo = 0x20,
        eKindM2Doodad = 0x40,
    };

    uint32_t out_ptr_;
    KindFlags flags_;
    uint16_t link_count_;
    Flags light_flags_;
    TSLink<void> registry_link_;
    TSExplicitList<CMapBaseObjLink<>> list_;

    HOOKKIT_HOOK_HANDLE(growOwnerBounds, 0x007B4FA0, hookkit::Conv::eCdecl, void, const void*, CMapBaseObj*);
};

static_assert(sizeof(CMapBaseObj) == 0x24);
