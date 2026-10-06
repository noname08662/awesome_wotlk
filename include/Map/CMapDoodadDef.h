#pragma once

#include <cstdint>

#include "Heap/CObjectHeapList.h"
#include "Lib/Storm.h"
#include "Map/CMapPlaceableObj.h"
#include "Math/Primitives.h"

class CM2Model;
class CM2Lighting;

struct SMDoodadDef {
    uint32_t name_id;    // doodad filename table
    uint32_t unique_id;  // CMapDoodadDef::TSHT hash key
    Vec3f position;      // on-disk axes; axis-flipped + chunk-offset on load
    Vec3f rotation;      // degrees; axis-permuted across RotateAroundX/Y/Z on load
    uint16_t scale;      // fixed point, real = scale / 1024.0
    uint16_t flags;
};

static_assert(sizeof(SMDoodadDef) == 0x24);

class CMapDoodadDef : public CMapPlaceableObj {
public:
    ~CMapDoodadDef() override;                                       // 0
    CM2Lighting* selectLights(CM2Lighting*) override;                // 1
    int resolveWmoGroup(int) override;                               // 2
    char updateInteriorLight(int, int, uint16_t*, Vec3f*) override;  // 3

    static CObjectHeapList* doodadDefHeap() { return *reinterpret_cast<CObjectHeapList**>(0x00D25420); }

    inline static auto& TSHT = *reinterpret_cast<TSHashTable<CMapDoodadDef>*>(0x00D2545C);

    TSHashObject<CMapDoodadDef> hash_obj_;
    TSLink<CMapDoodadDef> group_link_;
    uint32_t _unkB0[4];
    Vec3f _unkC0;
    Vec3f _unkCC;
    Mat4f world_transform_;
    Mat4f _unk118;
    uint32_t sound_[6];  // ctor 0x7C2319, dtor 0x7C2360

    HOOKKIT_HOOK_HANDLE(getFromFile, 0x007BECD0, hookkit::Conv::eCdecl, CMapDoodadDef*, char*, SMDoodadDef*, Vec3f*);
    HOOKKIT_HOOK_HANDLE(spawn, 0x007BEB40, hookkit::Conv::eCdecl, CMapDoodadDef*, char*, Vec3f*, float, int);
    HOOKKIT_HOOK_HANDLE(loadModel, 0x007BDA70, hookkit::Conv::eCdecl, CM2Model*, char*, CMapDoodadDef*, int, int);
};

static_assert(sizeof(CMapDoodadDef) == 0x170);
