#pragma once

#include <hookkit/hookkit.h>

#include <array>
#include <ranges>
#include <string>

#include "BaseTypes.h"

#include "ObjectManager/ObjectManager.h"
#include "Widget/CSimpleFrame.h"

struct LuaState;
using LuaNumber = double;

namespace lua {
inline LuaState* getLuaState() { return reinterpret_cast<LuaState* (*)()>(0x00817DB0)(); }

inline int getLuaRefErrorHandler() { return *reinterpret_cast<int*>(0x00AF576C); }

using LuaCFunction = int (*)(LuaState*);

struct LuaLReg {
    const char* name;
    LuaCFunction func;
};

inline constexpr int kTNone = -1;
inline constexpr int kTNil = 0;
inline constexpr int kTBoolean = 1;
inline constexpr int kTLightuserdata = 2;
inline constexpr int kTNumber = 3;
inline constexpr int kTString = 4;
inline constexpr int kTTable = 5;
inline constexpr int kTFunction = 6;
inline constexpr int kTUserdata = 7;
inline constexpr int kTThread = 8;

inline constexpr int kRegistryindex = -10000;
inline constexpr int kEnvironindex = -10001;
inline constexpr int kGlobalsindex = -10002;

constexpr int upvalueIndex(int i) { return kGlobalsindex - i; }

using FnLuaLChecktype = void (*)(LuaState*, int, int);
using FnLuaLChecklstring = const char* (*)(LuaState*, int, size_t*);
using FnLuaLChecknumber = LuaNumber (*)(LuaState*, int);
using FnLuaTouserdata = void* (*)(LuaState*, int);
using FnLuaTonumber = double (*)(LuaState*, int);
using FnLuaTolstring = const char* (*)(LuaState*, int, size_t*);
using FnLuaToboolean = bool (*)(LuaState*, int);
using FnLuaPushstring = void (*)(LuaState*, const char*);
using FnLuaPushboolean = void (*)(LuaState*, bool);
using FnLuaPushvalue = void (*)(LuaState*, int);
using FnLuaPushnumber = void (*)(LuaState*, LuaNumber);
using FnLuaPushlightuserdata = void (*)(LuaState*, void*);
using FnLuaPushcclosure = void (*)(LuaState*, LuaCFunction, int);
using FnLuaPushnil = void (*)(LuaState*);
using FnLuaRawseti = void (*)(LuaState*, int, int);
using FnluaRawGetI = void (*)(LuaState*, int, int);
using FnLuaRawset = void (*)(LuaState*, int);
using FnLuaRawget = void (*)(LuaState*, int);
using FnLuaSetfield = void (*)(LuaState*, int, const char*);
using FnLuaGetfield = void (*)(LuaState*, int, const char*);
using FnLuaNext = int (*)(LuaState*, int);
using FnLuaInsert = void (*)(LuaState*, int);
using FnLuaGettop = int (*)(LuaState*);
using FnLuaSettop = void (*)(LuaState*, int);
using FnLuaObjlen = int (*)(LuaState*, int);
using FnLuaType = int (*)(LuaState*, int);
using FnLuaPcall = int (*)(LuaState*, int, int, int);
using FnLuaGetParamValue = int (*)(LuaState*, int, int);
using FnLuaCreatetable = void (*)(LuaState*, int, int);
using FnLuaNewuserdata = void* (*)(LuaState*, size_t);
using FnLuaSetmetatable = int (*)(LuaState*, int);
using FnLuaLSetError = void(__cdecl*)(LuaState*, int);
using FnLuaLFormatError = void(__cdecl*)(LuaState*, const char*, va_list);
using FnLuaLSetStatus = void(__cdecl*)(LuaState*, int);
using FnLuaLThrow = void(__cdecl*)(LuaState*);

[[noreturn]]
inline void error(LuaState* l, const char* str, ...) {
    va_list va;
    va_start(va, str);

    reinterpret_cast<FnLuaLSetError>(0x0084F210)(l, 1);
    reinterpret_cast<FnLuaLFormatError>(0x0084E3A0)(l, str, va);
    reinterpret_cast<FnLuaLSetStatus>(0x0084EF90)(l, 2);
    reinterpret_cast<FnLuaLThrow>(0x0084EF30)(l);

    va_end(va);
    std::unreachable();
}

inline void throwError(LuaState* l, const char* error_text) {
    std::array<char, 0x400> dest{};
    const char* err = framescript::getCurrentFunction{}(l, dest.data(), dest.size());
    lua::error(l, error_text, err);
}

inline void checkType(LuaState* l, int idx, int t) { reinterpret_cast<FnLuaLChecktype>(0x0084F960)(l, idx, t); }

inline const char* checkLString(LuaState* l, int idx, size_t* len) {
    return reinterpret_cast<FnLuaLChecklstring>(0x0084F9F0)(l, idx, len);
}

inline LuaNumber checkNumber(LuaState* l, int idx) { return reinterpret_cast<FnLuaLChecknumber>(0x0084FAB0)(l, idx); }

inline void* toUserdata(LuaState* l, int idx) { return reinterpret_cast<FnLuaTouserdata>(0x0084E1C0)(l, idx); }

inline double toNumber(LuaState* l, int n_param) { return reinterpret_cast<FnLuaTonumber>(0x0084E030)(l, n_param); }

inline const char* toLString(LuaState* l, int idx, size_t* len) {
    return reinterpret_cast<FnLuaTolstring>(0x0084E0E0)(l, idx, len);
}

inline bool toBoolean(LuaState* l, int idx) { return reinterpret_cast<FnLuaToboolean>(0x0084E0B0)(l, idx); }

inline void pushString(LuaState* l, const char* str) { reinterpret_cast<FnLuaPushstring>(0x0084E350)(l, str); }

inline void pushBoolean(LuaState* l, bool b) { reinterpret_cast<FnLuaPushboolean>(0x0084E4D0)(l, b); }

inline void pushValue(LuaState* l, int idx) { reinterpret_cast<FnLuaPushvalue>(0x0084DE50)(l, idx); }

inline void pushNumber(LuaState* l, LuaNumber v) { reinterpret_cast<FnLuaPushnumber>(0x0084E2A0)(l, v); }

inline void pushLightuserdata(LuaState* l, void* data) {
    reinterpret_cast<FnLuaPushlightuserdata>(0x0084E500)(l, data);
}

inline void pushCClosure(LuaState* l, LuaCFunction func, int c) {
    reinterpret_cast<FnLuaPushcclosure>(0x0084E400)(l, func, c);
}

inline void pushNil(LuaState* l) { reinterpret_cast<FnLuaPushnil>(0x0084E280)(l); }

inline void rawSetI(LuaState* l, int idx, int pos) { reinterpret_cast<FnLuaRawseti>(0x0084EA00)(l, idx, pos); }

inline void rawGetI(LuaState* l, int idx, int pos) { reinterpret_cast<FnluaRawGetI>(0x0084E670)(l, idx, pos); }

inline void rawSet(LuaState* l, int idx) { reinterpret_cast<FnLuaRawset>(0x0084E970)(l, idx); }

inline void rawGet(LuaState* l, int idx) { reinterpret_cast<FnLuaRawget>(0x0084E600)(l, idx); }

inline void setField(LuaState* l, int idx, const char* str) {
    reinterpret_cast<FnLuaSetfield>(0x0084E900)(l, idx, str);
}

inline void getField(LuaState* l, int idx, const char* str) {
    reinterpret_cast<FnLuaGetfield>(0x0084E590)(l, idx, str);
}

inline int next(LuaState* l, int idx) { return reinterpret_cast<FnLuaNext>(0x0084EF50)(l, idx); }

inline void insert(LuaState* l, int idx) { reinterpret_cast<FnLuaInsert>(0x0084DCC0)(l, idx); }

inline int getTop(LuaState* l) { return reinterpret_cast<FnLuaGettop>(0x0084DBD0)(l); }

inline void setTop(LuaState* l, int idx) { reinterpret_cast<FnLuaSettop>(0x0084DBF0)(l, idx); }

inline int objLen(LuaState* l, int idx) { return reinterpret_cast<FnLuaObjlen>(0x0084E150)(l, idx); }

inline int type(LuaState* l, int idx) { return reinterpret_cast<FnLuaType>(0x0084DEB0)(l, idx); }

inline int pcall(LuaState* l, int argn, int retn, int eh) {
    return reinterpret_cast<FnLuaPcall>(0x0084EC50)(l, argn, retn, eh);
}

inline int getParamValue(LuaState* l, int idx, int def) {
    return reinterpret_cast<FnLuaGetParamValue>(0x00815500)(l, idx, def);
}

inline void createTable(LuaState* l, int narr, int nrec) {
    reinterpret_cast<FnLuaCreatetable>(0x0084E6E0)(l, narr, nrec);
}

inline void* newUserdata(LuaState* l, size_t size) { return reinterpret_cast<FnLuaNewuserdata>(0x0084F0F0)(l, size); }

inline int setMetatable(LuaState* l, int idx) { return reinterpret_cast<FnLuaSetmetatable>(0x0084EA90)(l, idx); }

inline constexpr hookkit::WildAbi<2> kGetObjectThisAbi = {
    {hookkit::ArgLoc::inReg(hookkit::Reg::eSi), hookkit::ArgLoc::onStack(0)}};

HOOKKIT_HOOK_WILD_HANDLE(
    getObjectThis, 0x004A81B0, hookkit::Conv::eUsercall, CSimpleFrame*, kGetObjectThisAbi, LuaState*, int);

inline int frameObjectType() {
    int& type = *reinterpret_cast<int*>(0x00B49984);
    if (type == 0) { type = ++*reinterpret_cast<int*>(0x00D3F778); }
    return type;
}

inline CSimpleFrame* toFrame(LuaState* l) { return getObjectThis{}(l, frameObjectType()); }

inline void pushframe(LuaState* l, CSimpleFrame* frame) { rawGetI(l, kRegistryindex, frame->getRefTable()); }

inline void pop(LuaState* l, int n) { setTop(l, -(n)-1); }

inline void newTable(LuaState* l) { createTable(l, 0, 0); }

inline void pushCFunction(LuaState* l, LuaCFunction f) { pushCClosure(l, f, 0); }

inline bool isFunction(LuaState* l, int n) { return type(l, n) == kTFunction; }

inline bool isTable(LuaState* l, int n) { return type(l, n) == kTTable; }

inline bool isLightuserdata(LuaState* l, int n) { return type(l, n) == kTLightuserdata; }

inline bool isNil(LuaState* l, int n) { return type(l, n) == kTNil; }

inline bool isString(LuaState* l, int n) { return type(l, n) == kTString; }

inline bool isNumber(LuaState* l, int n) { return type(l, n) == kTNumber; }

inline bool isBoolean(LuaState* l, int n) { return type(l, n) == kTBoolean; }

inline bool isThread(LuaState* l, int n) { return type(l, n) == kTThread; }

inline bool isNone(LuaState* l, int n) { return type(l, n) == kTNone; }

inline bool isSserdata(LuaState* l, int n) {
    int t = type(l, n);
    return (t == kTUserdata || t == kTLightuserdata);
}

inline bool isNoneOrNil(LuaState* l, int n) {
    int t = type(l, n);
    return (t == kTNone || t == kTNil);
}

inline void setGlobal(LuaState* l, const char* s) { setField(l, kGlobalsindex, s); }

inline void getGlobal(LuaState* l, const char* s) { getField(l, kGlobalsindex, s); }

inline const char* toString(LuaState* l, int i) { return toLString(l, i, nullptr); }

inline const char* checkString(LuaState* l, int i) { return checkLString(l, i, nullptr); }

inline CSimpleFrame* toFrameSilent(LuaState* l, int idx) {
    rawGetI(l, idx, 0);
    auto frame = static_cast<CSimpleFrame*>(toUserdata(l, -1));
    pop(l, 1);
    return frame;
}

inline void wipe(LuaState* l, int idx) {
    if (idx < 0) { idx = getTop(l) + idx + 1; }
    pushNil(l);  // push first key
    while (next(l, idx)) {
        // pushes key, value
        pop(l, 1);       // pop value, leave key
        pushNil(l);      // push key, nil
        rawSet(l, idx);  // table[key] = nil (pops key and nil)
        pushNil(l);      // prepare next iteration (push nil as key)
    }
}

inline void pushGuid(LuaState* l, guid_t guid) {
    char buf[24];
    object_mgr::guid2Str{}(guid, buf);
    pushString(l, buf);
}

inline bool checkSlashCommandExists(const char* cmd_key) {
    LuaState* l = getLuaState();
    if (!l || !cmd_key) { return false; }

    getGlobal(l, "SlashCmdList");
    if (isTable(l, -1)) {
        getField(l, -1, cmd_key);
        if (!isNil(l, -1)) {
            pop(l, 2);
            return true;
        }
        pop(l, 1);
    }
    pop(l, 1);

    std::string global_var = "SLASH_" + std::string(cmd_key) + "1";
    getGlobal(l, global_var.c_str());
    bool exists = !isNil(l, -1);
    pop(l, 1);
    return exists;
}

inline void registerSlashCommand(const char* cmd_key, const char* slash_str, LuaCFunction func) {
    LuaState* l = getLuaState();
    if (!l || !cmd_key || !slash_str) { return; }

    if (checkSlashCommandExists(cmd_key)) { return; }

    pushCFunction(l, func);
    getGlobal(l, "SlashCmdList");
    if (isTable(l, -1)) {
        pushValue(l, -2);
        setField(l, -2, cmd_key);
    }
    pop(l, 2);

    std::string global_var = "SLASH_" + std::string(cmd_key) + "1";
    pushString(l, slash_str);
    setGlobal(l, global_var.c_str());
}
}  // namespace lua
