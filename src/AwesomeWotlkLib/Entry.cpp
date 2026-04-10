#include "BugFixes.h"
#include "D3D.h"
#include "Camera.h"
#include "CommandLine.h"
#include "NamePlates.h"
#include "Misc.h"
#include "Hooks.h"
#include "Inventory.h"
#include "Item.h"
#include "MSDF.h"
#include "Lua.h"
#include "Spell.h"
#include "UnitAPI.h"
#include "VoiceChat.h"
#include <Windows.h>
#include <Detours/detours.h>

namespace {
    int lua_debugbreak(lua_State* L) {
        if (IsDebuggerPresent()) {
            DebugBreak();
        }
        return 0;
    }

    int lua_openawesomewotlk(lua_State* L) {
        Lua::lua_pushnumber(L, 1.0);
        Lua::lua_setglobal(L, "AwesomeWotlk");

#ifdef _DEBUG
        Lua::lua_pushcfunction(L, lua_debugbreak);
        Lua::lua_setglobal(L, "debugbreak");
#endif
        return 0;
    }

    void OnAttach() {
        // Invalid function pointer hack
        *reinterpret_cast<DWORD*>(0x00D415B8) = 1;
        *reinterpret_cast<DWORD*>(0x00D415BC) = 0x7FFFFFFF;

        // TOS/EULA Acceptance
        *reinterpret_cast<DWORD*>(0x00B6AF54) = 1; // TOSAccepted
        *reinterpret_cast<DWORD*>(0x00B6AF5C) = 1; // EULAAccepted

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        Hooks::initialize();
        D3D::initialize();
        Camera::initialize();
        BugFixes::initialize();
        CommandLine::initialize();
        Inventory::initialize();
        Item::initialize();
        MSDF::initialize();
        NamePlates::initialize();
        Misc::initialize();
        UnitAPI::initialize();
        Spell::initialize();
        VoiceChat::initialize();

        DetourTransactionCommit();

        Hooks::FrameXML::registerLuaLib(lua_openawesomewotlk);
    }
}

static BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        OnAttach();
    }
    return TRUE;
}