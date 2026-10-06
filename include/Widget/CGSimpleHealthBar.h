#pragma once

#include "Widget/CSimpleStatusBar.h"

class CGSimpleHealthBar : public CSimpleStatusBar {
public:
    ~CGSimpleHealthBar() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override;                                           // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    void getScriptTime(LuaState* l, double* out_total_time, uint32_t* out_call_count,
        int include_children) override;  // 22

    void unregisterRegion(CSimpleRegion* region) override;  // 24
    void onUpdate(float elapsed) override;                  // 30

    void setValue(float value) override;                 // 57
    uint32_t updateVertexColor(Vec4u8* color) override;  // 58

    uint32_t _alignment;
    guid_t tracking_guid_;
    uint32_t flag_;
};
