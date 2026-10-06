#include <libloaderapi.h>
#include <minwindef.h>
#include <windows.h>

#include <cassert>

#include "BugFixes.h"
#include "Camera.h"
#include "CommandLine.h"
#include "D3D.h"
#include "Extensions.h"
#include "Inventory.h"
#include "Item.h"
#include "MSDF.h"
#include "Misc.h"
#include "NamePlates.h"
#include "Spell.h"
#include "UnitAPI.h"
#include "VoiceChat.h"

#include "include/Lib/Lua.h"

namespace {
constexpr DWORD kStartupRetryBudgetMs = 5000;

#ifdef _DEBUG
int luaDebugBreak(LuaState*) {
    if (IsDebuggerPresent()) { DebugBreak(); }
    return 0;
}
#endif

int luaOpenAwesomeWotlk(LuaState* l) {
    lua::pushNumber(l, 38);
    lua::setGlobal(l, "AwesomeWotlk");

#ifdef _DEBUG
    lua::pushCFunction(l, luaDebugBreak);
    lua::setGlobal(l, "debugbreak");
#endif
    return 0;
}

void onAttach() {
    // invalid function pointer hack
    *reinterpret_cast<DWORD*>(0x00D415B8) = 1;
    *reinterpret_cast<DWORD*>(0x00D415BC) = 0x7FFFFFFF;

    *reinterpret_cast<DWORD*>(0x00B6AF54) = 1;  // TOSAccepted
    *reinterpret_cast<DWORD*>(0x00B6AF5C) = 1;  // EULAAccepted

    hookkit::HookTransaction tx{hookkit::RetryBudget{kStartupRetryBudgetMs}};

    extensions::initialize(tx);
    d3d::initialize(tx);
    camera::initialize(tx);
    bug_fixes::initialize(tx);
    command_line::initialize(tx);
    inventory::initialize(tx);
    item::initialize(tx);
    msdf::initialize(tx);
    name_plates::initialize(tx);
    misc::initialize(tx);
    unit_api::initialize(tx);
    spell::initialize(tx);
    voice_chat::initialize(tx);

    if (tx.commit() != NO_ERROR) {
        assert(false && "AwesomeWotlk: startup hooks failed to install, the mod stays disabled");
        return;
    }

    extensions::console::kLuaLibRegistry->add(luaOpenAwesomeWotlk);
}
}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        onAttach();
    }
    return TRUE;
}
