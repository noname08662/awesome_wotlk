#pragma once

#include "FrameScript/CScriptRegion.h"

struct CRenderBatch;

class CSimpleRegion : public CScriptRegion {
public:
    ~CSimpleRegion() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override = 0;                                       // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    CSimpleFrame* getScriptObjectParent() override;       // 5
    bool isMatchingObjectType(const char* str) override;  // 6
    const char* getObjectTypeStr() override;              // 7

    virtual void onColorChanged(int is_color_changed);  // 21
    virtual void nullsub();                             // 22
    virtual void draw(CRenderBatch*) = 0;               // 23
    virtual void updateGeometry();                      // 24
    virtual void onFrameChanged();                      // 25

    enum RegionFlags : uint8_t {
        eFlagNone = 0x0,
        eFlagLayerMask = 0xF,
        eFlagShown = 0x10,
        eFlagDrawLinked = 0x20,
        eFlagTransformDirty = 0x40,
        eFlagAlphaDirty = 0x80,
        eFlagDirtyMask = 0xC0,
        eFlagStateMask = 0x3F,
    };

    int32_t num_alpha_;
    uint8_t alphas_[4];
    int32_t num_color_;
    Vec4u8 color_[4];
    TSLink<CSimpleRegion> link1_;
    TSLink<CSimpleRegion> link2_;
    RegionFlags flag_;
    uint8_t alpha_;
    char _pad[2];
    uint32_t _unk;
};

static_assert(sizeof(CSimpleRegion) == 0xD4);
