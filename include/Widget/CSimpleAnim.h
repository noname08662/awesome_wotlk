#pragma once

#include "BaseTypes.h"

#include "FrameScript/CScriptObject.h"
#include "Lib/Storm.h"

class CScriptRegion;

class CSimpleAnim : public CScriptObject {
public:
    ~CSimpleAnim() override;  // 0

    const char* getName() override;                                          // 1
    void* getMetaTable() override;                                           // 2
    ScriptIx* getScriptByName(const char* name, const char** out) override;  // 3
    bool isObjectType(int type) override;                                    // 4

    CSimpleFrame* getScriptObjectParent() override;       // 5
    bool isMatchingObjectType(const char* str) override;  // 6
    const char* getObjectTypeStr() override;              // 7

    virtual void loadXML(XMLNode* xml_node, CStatus* status);  // 8
    virtual void onLoad();                                     // 9
    virtual void getScriptTime(LuaState* l, double* out_total_time, uint32_t* out_call_count,
        int include_children);  // 10
    virtual void nullsub0();    // 11
    virtual void nullsub1();    // 12

    enum PlayState : uint8_t {
        eStopped = 0u,
        ePlaying = 1u,
        ePaused = 2u,
    };

    enum PlayDirection : uint8_t {
        eNone = 0u,
        eRepeat = 1u,
        eBounce = 2u,
    };

    enum Scripts : uint8_t {
        eOnLoad,
        eOnPlay,
        eOnPause,
        eOnStop,
        eOnFinished,
        eOnUpdate,
        eScriptsCount,
    };

    TSLink<CSimpleAnim> link_;
    CLayoutFrame* parent_;
    int8_t flag_;
    char _pad1[3];
    int8_t sequence_index_;
    int8_t flag3_;
    char _pad2[2];
    PlayState play_state_;
    PlayDirection play_direction_;
    char _pad3[2];
    unk_t _alignment;
    ScriptIx scripts_[eScriptsCount];
    float start_delay_;
    float end_delay_;
    float duration_;
    float _field_78;
    float update_interval_;
    float update_time_acc_;
    float elapsed_time_;
    float total_progress_;
    float linear_progress_;
    float curve_progress_;
    void* interpolator_;
};

static_assert(sizeof(CSimpleAnim) == 0x98);

class CSimpleAnimGroup : public CScriptObject {
public:
    struct SIMPLEANIMNODE {
        TSLink<SIMPLEANIMNODE> link;
        CSimpleAnim* anim;
    };

    static_assert(sizeof(SIMPLEANIMNODE) == 0xC);

    enum GroupFlags : uint16_t {
        eAnimGroupFlagPlaying = 0x01,
        eAnimGroupFlagPendingUpdate = 0x02,
        eAnimGroupFlagStopRequested = 0x04,
        eAnimGroupFlagUnk8 = 0x08,
        eAnimGroupFlagIsStopping = 0x20,
    };

    unk_t _unk20[4];
    CScriptRegion* parent_region_;
    TSExplicitList<CSimpleAnim> master_list_;
    TSGrowableArray<TSList<SIMPLEANIMNODE>*> anims_;
    ScriptIx scripts_[CSimpleAnim::eScriptsCount];
    ScriptIx _unk_script;
    GroupFlags flags_;
    char _pad[2];
    CSimpleAnim::PlayDirection loop_type_;
    CSimpleAnim::PlayState play_state_;
    char _pad2[2];
    uint32_t current_order_idx_;
    float current_time_;
    float duration_;
    float time_elapsed_;
    float initial_offset_x_;
    float initial_offset_y_;
};

static_assert(sizeof(CSimpleAnimGroup) == 0xA8);
