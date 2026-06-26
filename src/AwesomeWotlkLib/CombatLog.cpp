// CombatLog.cpp
// Per-session combat-log file.
//
// WoW 3.3.5a opens "Logs\WoWCombatLog.txt" through the ANSI file API (CreateFileA) and
// appends, so every session lands in one growing file. This detours CreateFileA and rewrites
// that single path to a per-session name:
//
//     Logs\<YYYY-MM-DD-HH.MM.SS> WoWCombatLog.txt
//
// i.e. one file per game launch, matching the Ascension/Epoch client output.
//
// The redirect is on the ANSI variant on purpose: the 3.3.5a Wow.exe imports only CreateFileA
// for file I/O (no CreateFileW, no fopen), so a CreateFileW hook would never be invoked.

#include "CombatLog.h"
#include "Hooks.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

namespace {
HANDLE(WINAPI* CreateFileAFn)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE) = CreateFileA;

HANDLE WINAPI CreateFileA_hk(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES sa,
                             DWORD disp, DWORD flags, HANDLE tmpl)
{
    const char* hit;
    // Match the combat-log file specifically (require both "Logs" and the file name) so we
    // never touch screenshots, WTF, WoWChatLog.txt, etc.
    if (name && strstr(name, "Logs") && (hit = strstr(name, "WoWCombatLog.txt"))) {
        static char stamp[40] = { 0 };          // built once per process == one file per launch
        if (!stamp[0]) {
            SYSTEMTIME t; GetLocalTime(&t);
            sprintf(stamp, "%04d-%02d-%02d-%02d.%02d.%02d ",
                    t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
        }
        char buf[MAX_PATH];
        size_t pre = (size_t)(hit - name);      // length of the "...\Logs\" prefix
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
}

void CombatLog::initialize()
{
    Hooks::Detour(&CreateFileAFn, CreateFileA_hk);
}
