#include "BugFixes.h"
#include "Utils.h"
#include "Hooks.h"
#include <Windows.h>
#include <string>

namespace {
    using Alloc_t = void* (*)(size_t);
    const auto alloc = reinterpret_cast<Alloc_t>(0x00415074);

    using ClipboardGetString_t = const char* (*)(HWND);
    auto(*Clipboard_GetStringFn)(HWND) = reinterpret_cast<ClipboardGetString_t>(0x008726F0);

    using ClipboardSetString_t = int(*)(const char*, HWND);
    auto(*Clipboard_SetStringFn)(const char*, HWND) = reinterpret_cast<ClipboardSetString_t>(0x008727E0);

    const char* Clipboard_GetStringHk(HWND hwnd) {
        std::string str = GetFromClipboardU8(hwnd);
        char* buf = static_cast<char*>(alloc(str.size() + 1));
        if (buf) memcpy(buf, str.c_str(), str.size() + 1);
        return buf;
    }
    int Clipboard_SetStringHk(const char* buf, HWND hwnd) { return CopyToClipboardU8(buf, hwnd); }
}

void BugFixes::initialize() {
    Hooks::Detour(&Clipboard_GetStringFn, Clipboard_GetStringHk);
    Hooks::Detour(&Clipboard_SetStringFn, Clipboard_SetStringHk);
}