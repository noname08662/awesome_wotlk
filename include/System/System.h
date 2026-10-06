#pragma once

#include <hookkit/hookkit.h>

namespace os {
inline HWND getGameWindow() { return *reinterpret_cast<HWND*>(0x00D41620); }

HOOKKIT_HOOK_HANDLE(getAsyncTimeMs, 0x0086AE20, hookkit::Conv::eCdecl, uint64_t);
HOOKKIT_HOOK_HANDLE(clipboardGetStr, 0x008726F0, hookkit::Conv::eCdecl, const char*, HWND);
HOOKKIT_HOOK_HANDLE(clipboardSetStr, 0x008727E0, hookkit::Conv::eCdecl, int, const char*, HWND);
HOOKKIT_HOOK_HANDLE(malloc, 0x00415074, hookkit::Conv::eCdecl, void*, size_t);
}  // namespace os
