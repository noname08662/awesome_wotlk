// CombatLog.cpp
// Per-session combat-log file.
//
// WoW 3.3.5a opens "Logs\WoWCombatLog.txt" and appends, so every session lands in one growing
// file. This detours the file-open call and rewrites that single path to a per-session name:
//
//     Logs\<YYYY-MM-DD-HH.MM.SS> WoWCombatLog.txt
//
// i.e. one file per game launch, matching the Ascension/Epoch client output.
//
// Both the ANSI (CreateFileA) and wide (CreateFileW) variants are hooked, because different
// 3.3.5a client builds open the combat log through different APIs. A given Wow.exe uses one or
// the other; hooking both makes the redirect work regardless of which. The single shared
// timestamp keeps every open within a session pointed at the same file.

#include "CombatLog.h"
#include "Hooks.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

namespace {
// Session timestamp "YYYY-MM-DD-HH.MM.SS " built once per process == one file per launch.
const char* sessionStamp()
{
    static char s[40] = { 0 };
    if (!s[0]) {
        SYSTEMTIME t; GetLocalTime(&t);
        sprintf(s, "%04d-%02d-%02d-%02d.%02d.%02d ",
                t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    }
    return s;
}

// ---- ANSI ----
HANDLE(WINAPI* CreateFileAFn)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE) = CreateFileA;
HANDLE WINAPI CreateFileA_hk(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa,
                             DWORD disp, DWORD flags, HANDLE tmpl)
{
    const char* hit;
    if (name && strstr(name, "Logs") && (hit = strstr(name, "WoWCombatLog.txt"))) {
        const char* stamp = sessionStamp();
        char buf[MAX_PATH];
        size_t pre = (size_t)(hit - name);
        if (pre + strlen(stamp) + sizeof("WoWCombatLog.txt") < sizeof(buf)) {
            memcpy(buf, name, pre);
            buf[pre] = 0;
            strcat(buf, stamp);
            strcat(buf, "WoWCombatLog.txt");
            return CreateFileAFn(buf, access, share, sa, disp, flags, tmpl);
        }
    }
    return CreateFileAFn(name, access, share, sa, disp, flags, tmpl);
}

// ---- wide ----
HANDLE(WINAPI* CreateFileWFn)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE) = CreateFileW;
HANDLE WINAPI CreateFileW_hk(LPCWSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa,
                             DWORD disp, DWORD flags, HANDLE tmpl)
{
    const wchar_t* hit;
    if (name && wcsstr(name, L"Logs") && (hit = wcsstr(name, L"WoWCombatLog.txt"))) {
        const char* stamp = sessionStamp();          // ASCII-only -> widen by simple cast
        wchar_t wstamp[40]; size_t n = 0;
        while (stamp[n] && n < 39) { wstamp[n] = (wchar_t)(unsigned char)stamp[n]; ++n; }
        wstamp[n] = 0;
        wchar_t buf[MAX_PATH];
        size_t pre = (size_t)(hit - name);
        if (pre + n + (sizeof("WoWCombatLog.txt")) < (sizeof(buf) / sizeof(wchar_t))) {
            wmemcpy(buf, name, pre);
            buf[pre] = 0;
            wcscat(buf, wstamp);
            wcscat(buf, L"WoWCombatLog.txt");
            return CreateFileWFn(buf, access, share, sa, disp, flags, tmpl);
        }
    }
    return CreateFileWFn(name, access, share, sa, disp, flags, tmpl);
}
}

void CombatLog::initialize()
{
    Hooks::Detour(&CreateFileAFn, CreateFileA_hk);
    Hooks::Detour(&CreateFileWFn, CreateFileW_hk);
}
