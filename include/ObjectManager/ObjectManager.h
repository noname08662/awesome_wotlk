#pragma once

#include <cstdint>
#include <type_traits>

#include "BaseTypes.h"

#include "ObjectManager/ObjectManagerEnums.h"

namespace object_mgr {
template <typename T>
using GetFuncPtr = T* (*)(guid_t, TypeMask);

HOOKKIT_HOOK_HANDLE(enumObjs, 0x004D4B30, hookkit::Conv::eCdecl, int, int (*)(guid_t, void*), void*);

template <typename F>
bool enumObjects(F&& func) {
    struct Wrapper {
        static int __cdecl callback(uint64_t guid, void* udata) {
            auto& f = *static_cast<std::remove_reference_t<F>*>(udata);
            return f(guid) ? 1 : 0;
        }
    };

    return enumObjs{}(&Wrapper::callback, &func) != 0;
}

template <typename T>
T* get(guid_t guid, TypeMask flags) {
    return (reinterpret_cast<GetFuncPtr<T>>(0x004D4DB0))(guid, flags);
}

HOOKKIT_HOOK_HANDLE(str2Guid, 0x0074D120, hookkit::Conv::eCdecl, guid_t, const char*);
HOOKKIT_HOOK_HANDLE(guid2Str, 0x0074D0D0, hookkit::Conv::eCdecl, void, guid_t, char*);
HOOKKIT_HOOK_HANDLE(guidFromUnitId, 0x0060C1C0, hookkit::Conv::eCdecl, guid_t, const char*);

inline guid_t string2Guid(const char* str) {
    if (!str) { return 0; }
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) { return str2Guid{}(str); }
    return guidFromUnitId{}(str);
}

HOOKKIT_HOOK_HANDLE(handleObjectTrackChange, 0x0051F790, hookkit::Conv::eCdecl, void, guid_t, guid_t);

inline guid_t getTargetGuid() { return *reinterpret_cast<guid_t*>(0x00BD07B0); }

inline guid_t getPlayerGuid() { return reinterpret_cast<guid_t (*)()>(0x004D3790)(); }
}  // namespace object_mgr
