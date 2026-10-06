#pragma once

#include "BaseTypes.h"

#include "Widget/CSimpleFrame.h"

class CSimpleStatusBar : public CSimpleFrame {
public:
    enum BarFlags {
        eStatusbarFlagDirty = 0x1,
        eStatusbarFlagMinmaxSet = 0x2,
        eStatusbarFlagValueSet = 0x4,
        eStatusbarFlagUnk = 0x8,
    };

    ~CSimpleStatusBar() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override = 0;                                       // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    void update() override;  // 10

    void getScriptTime(LuaState* l, double* out_total_time, uint32_t* out_call_count,
        int include_children) override;  // 22

    void unregisterRegion(CSimpleRegion* region) override;  // 24
    void onUpdate(float elapsed) override;                  // 30

    virtual void setValue(float value);                 // 57
    virtual uint32_t updateVertexColor(Vec4u8* color);  // 58
    virtual float getProgress();                        // 59

    BarFlags flags_;
    Vec2f min_max_;
    float value_;
    CSimpleTexture* tex_;
    unk_t _unk;
    ScriptIx on_value_changed_;
    ScriptIx on_minmax_changed_;
};

static_assert(sizeof(CSimpleStatusBar) == 0x2C4);
