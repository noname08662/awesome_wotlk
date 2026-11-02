#pragma once

#include <hookkit/hookkit.h>

namespace os {
inline HWND getGameWindow() { return *reinterpret_cast<HWND*>(0x00D41620); }

HOOKKIT_HOOK_HANDLE(getAsyncTimeMs, 0x0086AE20, hookkit::Conv::eCdecl, uint64_t);
HOOKKIT_HOOK_HANDLE(getAsyncTimeClocks, 0x0086AE30, hookkit::Conv::eCdecl, uint64_t);
HOOKKIT_HOOK_HANDLE(getAsyncClocksPerSecond, 0x0086AE70, hookkit::Conv::eCdecl, int64_t);
HOOKKIT_HOOK_HANDLE(validateFunctionPointer, 0x0086B5A0, hookkit::Conv::eCdecl, const void*, const void*);
HOOKKIT_HOOK_HANDLE(clipboardGetStr, 0x008726F0, hookkit::Conv::eCdecl, const char*, HWND);
HOOKKIT_HOOK_HANDLE(clipboardSetStr, 0x008727E0, hookkit::Conv::eCdecl, int, const char*, HWND);
HOOKKIT_HOOK_HANDLE(malloc, 0x00415074, hookkit::Conv::eCdecl, void*, size_t);

inline void* const kInvalidFile = reinterpret_cast<void*>(-1);
HOOKKIT_HOOK_HANDLE(
    createFile, 0x00461FA0, hookkit::Conv::eCdecl, void*, const char*, uint32_t, uint32_t, uint32_t, uint32_t);
HOOKKIT_HOOK_HANDLE(writeFile, 0x00461B90, hookkit::Conv::eCdecl, int, void*, const void*, uint32_t, uint32_t*);
HOOKKIT_HOOK_HANDLE(closeFile, 0x00461B00, hookkit::Conv::eCdecl, bool, void*);
}  // namespace os
