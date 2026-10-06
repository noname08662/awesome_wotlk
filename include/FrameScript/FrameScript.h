#pragma once

#include <hookkit/hook.h>

#include <cstdarg>
#include <cstdint>

#include "BaseTypes.h"

#include "Common/Common.h"

struct LuaState;
class CScriptObject;

struct ScriptIx {
    int32_t script_id;
    const char* taint;
};

class FrameScriptObject : public CHandle {
public:
    ~FrameScriptObject() override;  // 0

    virtual const char* getName() = 0;                                      // 1
    virtual void* getMetaTable() = 0;                                       // 2
    virtual ScriptIx* getScriptByName(const char* name, const char** out);  // 3
    virtual bool isObjectType(int type) = 0;                                // 4

    int lua_ref_;
    ScriptIx on_event_;
};

static_assert(sizeof(FrameScriptObject) == 0x14);

namespace framescript {
namespace types {
template <std::uintptr_t Address>
[[nodiscard]]
int read() {
    return *reinterpret_cast<const int*>(Address);
}

inline int texture() { return read<0x00B4793C>(); }

inline int fontString() { return read<0x00B4792C>(); }

inline int region() { return read<0x00B49978>(); }

inline int object() { return read<0x00B4997C>(); }

inline int frame() { return read<0x00B49984>(); }

inline int font() { return read<0x00B499B0>(); }

inline int controlPoint() { return read<0x00B499E8>(); }

inline int animation() { return read<0x00B499F8>(); }

inline int translation() { return read<0x00B499F4>(); }

inline int rotation() { return read<0x00B499F0>(); }

inline int scale() { return read<0x00B499EC>(); }

inline int alpha() { return read<0x00B499E0>(); }

inline int path() { return read<0x00B499E4>(); }

inline int animationGroup() { return read<0x00B499DC>(); }

inline int model() { return read<0x00DCE428>(); }

inline int minimap() { return read<0x00BEBA60>(); }

inline int questPOIFrame() { return read<0x00C0D7C4>(); }

inline int playerModel() { return read<0x00C0E4D4>(); }

inline int dressUpModel() { return read<0x00C0E4F4>(); }

inline int tabardModel() { return read<0x00C0E520>(); }

inline int cooldown() { return read<0x00C2423C>(); }

inline int gameTooltip() { return read<0x00C5CF4C>(); }

inline int movieFrame() { return read<0x00DCE3D8>(); }

inline int statusBar() { return read<0x00DCE440>(); }

inline int checkButton() { return read<0x00DCE458>(); }

inline int editBox() { return read<0x00DCE470>(); }

inline int messageFrame() { return read<0x00DCE48C>(); }

inline int scrollingMessageFrame() { return read<0x00DCE4A4>(); }

inline int scrollFrame() { return read<0x00DCE4BC>(); }

inline int slider() { return read<0x00DCE4D4>(); }

inline int simpleHTML() { return read<0x00DCE4EC>(); }

inline int colorSelect() { return read<0x00DCE51C>(); }

inline int button() { return read<0x00DCE650>(); }
}  // namespace types

struct EventObject {
    inline static auto& TSHT = *reinterpret_cast<TSHashTable<EventObject>*>(0x00D3F7A8);

    struct EVENTLISTENERNODE {
        TSLink<EVENTLISTENERNODE> link;
        CScriptObject* obj;
    };

    TSHashObject<EventObject> hash_obj;
    TSExplicitList<EVENTLISTENERNODE> event_listener_list;
    TSExplicitList<EVENTLISTENERNODE> event_listener_list_deferred;
    TSExplicitList<EVENTLISTENERNODE> event_listener_list_deferred2;
    uint32_t _alignment;
    uint64_t profile_time;
    uint32_t count;
    uint32_t event_firing_count;
};

static_assert(sizeof(EventObject) == 0x50);

struct PEventObject {
    inline static auto& TSGRA = *reinterpret_cast<TSGrowableArray<PEventObject>*>(0x00D3F7D0);

    EventObject* obj;
};

static_assert(sizeof(PEventObject) == 0x4);

// list, count
HOOKKIT_HOOK_HANDLE(fillEvents, 0x0081B5F0, hookkit::Conv::eCdecl, void, const char**, size_t);

// event id, fmt, args
HOOKKIT_HOOK_HANDLE(fireEvent, 0x0081AC90, hookkit::Conv::eCdecl, void, int, const char*, va_list);
HOOKKIT_HOOK_HANDLE(paintCallback, 0x00495810, hookkit::Conv::eCdecl, int, int, int, int, float);
// event id, lua state, args already on the stack
HOOKKIT_HOOK_HANDLE(signalEvent, 0x0081AA00, hookkit::Conv::eCdecl, void, int, LuaState*, int);
// key, plural index, gender
HOOKKIT_HOOK_HANDLE(getText, 0x00819D40, hookkit::Conv::eCdecl, const char*, const char*, int, int);
HOOKKIT_HOOK_HANDLE(execute, 0x00819210, hookkit::Conv::eCdecl, int, const char*, const char*, const char*);

inline void event(int id, const char* format, ...) {
    va_list args;
    va_start(args, format);
    fireEvent{}(id, format, args);
    va_end(args);
}

HOOKKIT_HOOK_HANDLE(getTokensFromGuid, 0x0060BB70, hookkit::Conv::eCdecl, char**, guid_t*, size_t*);

HOOKKIT_HOOK_HANDLE(secureCmdOptionsParse, 0x00564AE0, hookkit::Conv::eCdecl, int, LuaState*);

HOOKKIT_HOOK_HANDLE(registerBNScriptFunctions, 0x00530F60, hookkit::Conv::eCdecl, void);

HOOKKIT_HOOK_HANDLE(getCurrentFunction, 0x00817EE0, hookkit::Conv::eCdecl, char*, LuaState*, char*, size_t);

}  // namespace framescript
