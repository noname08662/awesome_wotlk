#pragma once

#include "Lib/Storm.h"

struct CHandle {
    virtual ~CHandle() = default;
    uint32_t ref_count;

    // drops a ref, deletes at 0
    HOOKKIT_HOOK_HANDLE(close, 0x0047BF30, hookkit::Conv::eCdecl, void, CHandle*);
};

class CStatus {
public:
    enum Type {
        eStatusInfo = 0x0,
        eStatusWarning = 0x1,
        eStatusError = 0x2,
        eStatusFatal = 0x3,
        eStatusNumtypes = 0x4,
    };

    struct STATUSENTRY {
        char* text;
        Type severity;
        TSLink<STATUSENTRY> link;
    };

    static_assert(sizeof(STATUSENTRY) == 0x10);

    struct {
        CStatus*(__thiscall* sddtor)(CStatus*, int8_t free);
        void (*nullsub)();
        CStatus*(__thiscall* link)(CStatus*, CStatus* other);
        void (*add)(CStatus*, Type type, const char* format, ...);
        void (*print)(CStatus*, Type type, const char* format, ...);
    }* vmt_;

    TSExplicitList<STATUSENTRY> statuses_;
    Type highest_status_;

    static CStatus* get() { return *reinterpret_cast<CStatus**>(0x00AC0CE4); }
};

static_assert(sizeof(CStatus) == 0x14);

class RCString : CHandle {
public:
    const char* str_;
};

static_assert(sizeof(RCString) == 0xC);
