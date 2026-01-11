#include "BugFixes.h"
#include "D3D.h"
#include "CommandLine.h"
#include "NamePlates.h"
#include "Misc.h"
#include "Hooks.h"
#include "Inventory.h"
#include "Item.h"
#include "Fonts.h"
#include "Spell.h"
#include "UnitAPI.h"
#include "VoiceChat.h"
#include <Windows.h>
#include <Detours/detours.h>


static int lua_debugbreak(lua_State* L)
{
    if (IsDebuggerPresent())
        DebugBreak();
    return 0;
}

static int lua_openawesomewotlk(lua_State* L)
{
    lua_pushnumber(L, 1.f);
    lua_setglobal(L, "AwesomeWotlk");
#ifdef _DEBUG
    lua_pushcfunction(L, lua_debugbreak);
    lua_setglobal(L, "debugbreak");
#endif
    return 0;
}

static void OnAttach()
{
//#ifdef _DEBUG
//    system("pause");
//    FreeConsole();
//#endif

    // Invalid function pointer hack
    *(DWORD*)0x00D415B8 = 1;
    *(DWORD*)0x00D415BC = 0x7FFFFFFF;

    *(DWORD*)0x00B6AF54 = 1; // TOSAccepted = 1
    *(DWORD*)0x00B6AF5C = 1; // EULAAccepted = 1

    // Initialize modules
    DetourTransactionBegin();
    Hooks::initialize();
    D3D::initialize();
    BugFixes::initialize();
    CommandLine::initialize();
    Inventory::initialize();
    Item::initialize();
    Fonts::initialize();
    NamePlates::initialize();
    Misc::initialize();
    UnitAPI::initialize();
    Spell::initialize();
    VoiceChat::initialize();
    DetourTransactionCommit();

    // Register base
    Hooks::FrameXML::registerLuaLib(lua_openawesomewotlk);
}

static int __stdcall DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        OnAttach();
    return 1;
}
