#pragma once

#include <cstdint>

#include "FrameScript/CScriptObject.h"
#include "Lib/Storm.h"
#include "UI/CLayoutFrame.h"

class CSimpleFrame;
class CSimpleAnimGroup;

class CScriptRegion : public CScriptObject, public CLayoutFrame {
public:
    enum PivotMode : uint32_t {
        eBottomLeft = 0x0,
        eLeft = 0x1,
        eTopLeft = 0x2,
        eBottom = 0x3,
        eCenter = 0x4,
        eTop = 0x5,
        eBottomRight = 0x6,
        eRight = 0x7,
        eTopRight = 0x8,
    };

    ~CScriptRegion() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override = 0;                                       // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    CSimpleFrame* getScriptObjectParent() override;       // 5
    bool isMatchingObjectType(const char* str) override;  // 6
    const char* getObjectTypeStr() override;              // 7

    virtual void setParent(CSimpleFrame* parent);                                               // 8
    virtual CLayoutFrame* getLayout();                                                          // 9
    virtual void update();                                                                      // 10
    virtual void onAnimUpdate(float delta);                                                     // 11
    virtual void registerActiveAnimGroup(CSimpleFrame* parent);                                 // 12
    virtual void unregisterActiveAnimGroup(CLayoutFrame* parent);                               // 13
    virtual void stopAnims();                                                                   // 14
    virtual char onAnimate(float delta, int update_geom, int is_anim_playing);                  // 15
    virtual void onAnimFinished(int unused, int update_geom);                                   // 16
    virtual void setPosition(Vec2f* offset);                                                    // 17
    virtual int setRotation(PivotMode pivot_mode, const Vec2f* pivot_offset, float radians);    // 18
    virtual int setScale(PivotMode pivot_mode, const Vec2f* local_offset, const Vec2f* scale);  // 19
    virtual int setAlpha(int16_t alpha);                                                        // 20

    // CLayoutFrame
    void loadXML(XMLNode* node, CStatus* status) override;  // 1
    CLayoutFrame* getLayoutParent() override;               // 2

    CSimpleFrame* parent_;
    TSExplicitList<CSimpleAnimGroup>* anims_owned_;
    TSExplicitList<CSimpleAnimGroup>* anims_active_;
};

static_assert(sizeof(CScriptRegion) == 0xA0);
