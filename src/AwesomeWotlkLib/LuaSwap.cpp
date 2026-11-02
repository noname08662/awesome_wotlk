#include "LuaSwap.h"

#include <array>
#include <cassert>

#include "Extensions.h"

#include "deps/luajit/extension/lmemPool.h"
#include "deps/luajit/src/lauxlib.h"
#include "deps/luajit/src/lua.h"
#include "deps/luajit/src/lualib.h"

namespace {
constexpr DWORD kRetryBudgetMs = 5000;

struct Entry {
    template <typename Function>
    Entry(std::uintptr_t address, Function* function)
        : original{reinterpret_cast<void*>(address)}, replacement{reinterpret_cast<void*>(function)} {}

    void* original;
    void* replacement;
};

void swapVm() {
    // objects the original VM made point at its own statics (dummynode, nil object)
    const auto* const pool = *reinterpret_cast<void* const*>(0x00D3F788);   // FrameXML::LuaMemPool
    const auto* const state = *reinterpret_cast<void* const*>(0x00D3F78C);  // FrameXML::LuaState
    if (pool != nullptr || state != nullptr) {
        assert(false && "lua_swap: a lua_State already exists, the swap stays off");
        return;
    }

    // every public API entry of the original VM
    auto entries = std::to_array<Entry>({
        // lapi.c
        {0x0084DAB0, lua_checkstack},
        {0x0084DB10, lua_xmove},
        {0x0084DB90, lua_newthread},
        {0x0084DBD0, lua_gettop},
        {0x0084DBF0, lua_settop},
        {0x0084DC50, lua_remove},
        {0x0084DCC0, lua_insert},
        {0x0084DD70, lua_replace},
        {0x0084DE50, lua_pushvalue},
        {0x0084DEB0, lua_type},
        {0x0084DED0, lua_typename},
        {0x0084DEF0, lua_iscfunction},
        {0x0084DF20, lua_isnumber},
        {0x0084DF60, lua_isstring},
        {0x0084DF90, lua_rawequal},
        {0x0084DFE0, lua_lessthan},
        {0x0084E030, lua_tonumber},
        {0x0084E070, lua_tointeger},
        {0x0084E0B0, lua_toboolean},
        {0x0084E0E0, lua_tolstring},
        {0x0084E150, lua_objlen},
        {0x0084E1C0, lua_touserdata},
        {0x0084E1F0, lua_tothread},
        {0x0084E210, lua_topointer},
        {0x0084E280, lua_pushnil},
        {0x0084E2A0, lua_pushnumber},
        {0x0084E2D0, lua_pushinteger},
        {0x0084E300, lua_pushlstring},
        {0x0084E350, lua_pushstring},
        {0x0084E3A0, lua_pushvfstring},
        {0x0084E3D0, lua_pushfstring},
        {0x0084E400, lua_pushcclosure},
        {0x0084E4D0, lua_pushboolean},
        {0x0084E500, lua_pushlightuserdata},
        {0x0084E530, lua_pushthread},
        {0x0084E560, lua_gettable},
        {0x0084E590, lua_getfield},
        {0x0084E600, lua_rawget},
        {0x0084E670, lua_rawgeti},
        {0x0084E6E0, lua_createtable},
        {0x0084E730, lua_getmetatable},
        {0x0084E7A0, lua_getfenv},
        {0x0084E8D0, lua_settable},
        {0x0084E900, lua_setfield},
        {0x0084E970, lua_rawset},
        {0x0084EA00, lua_rawseti},
        {0x0084EA90, lua_setmetatable},
        {0x0084EB40, lua_setfenv},
        {0x0084EBF0, lua_call},
        {0x0084EC50, lua_pcall},
        {0x0084ECC0, lua_load},
        {0x0084ED00, lua_dump},
        {0x0084ED40, lua_status},
        {0x0084ED50, lua_gc},
        {0x0084EE50, lua_gcupdatesizes},
        {0x0084EE60, lua_gcgetsize},
        {0x0084EE70, lua_gcupdatetimes},
        {0x0084EE80, lua_gcgettime},
        {0x0084EE90, lua_gcgetfunctiontime},
        {0x0084EF20, lua_gcresettimes},
        {0x0084EF30, lua_error},
        {0x0084EF50, lua_next},
        {0x0084EF90, lua_concat},
        {0x0084F010, lua_setdecimalconversion},
        {0x0084F030, lua_convertdecimal},
        {0x0084F0D0, lua_settaint},
        {0x0084F0F0, lua_newuserdata},
        {0x0084F150, lua_getupvalue},
        // lauxlib.c
        {0x0084F210, luaL_where},
        {0x0084F280, luaL_error},
        {0x0084F2C0, luaL_checkstack},
        {0x0084F2F0, luaL_getmetafield},
        {0x0084F350, luaL_callmeta},
        {0x0084F3B0, luaL_findtable},
        {0x0084F520, luaL_prepbuffer},
        {0x0084F560, luaL_addlstring},
        {0x0084F5C0, luaL_pushresult},
        {0x0084F610, luaL_addvalue},
        {0x0084F6A0, luaL_buffinit},
        {0x0084F6C0, luaL_ref},
        {0x0084F7A0, luaL_unref},
        {0x0084F860, luaL_loadbuffer},
        {0x0084F8B0, luaL_argerror},
        {0x0084F960, luaL_checktype},
        {0x0084F9C0, luaL_checkany},
        {0x0084F9F0, luaL_checklstring},
        {0x0084FA50, luaL_optlstring},
        {0x0084FAB0, luaL_checknumber},
        {0x0084FB30, luaL_optnumber},
        {0x0084FB60, luaL_checkinteger},
        {0x0084FBD0, luaL_optinteger},
        {0x0084FC00, luaL_openlib},
        {0x0084FD20, luaL_checkoption},
        {0x0084FDD0, luaL_register},
        // ldebug.c
        {0x0084FE40, lua_getstack},
        {0x0084FF30, lua_getlocal},
        {0x00850A90, lua_getinfo},
        // lbitlib.c
        {0x00851010, luaopen_bit},
        // lmathlib.c
        {0x00851760, luaopen_math},
        // ltablib.c
        {0x008521F0, luaopen_table},
        // lstrlib.c
        {0x008540D0, luaopen_string},
        // lbaselib.c
        {0x00855060, luaopen_base},
        // lstate.c
        {0x00855370, lua_newstate},
        {0x00855510, lua_close},
        // extension/lmemPool.cpp
        {0x00855740, luaM_initPool},
        {0x008557D0, luaM_freePool},
        {0x008558E0, luaM_reallocPool},
        // ldo.c
        {0x00856150, lua_yield},
        {0x008568F0, lua_resume},
    });

    hookkit::HookTransaction tx{hookkit::RetryBudget{kRetryBudgetMs}};
    if (!tx) {
        assert(false && "lua_swap: no hook transaction, the swap stays off");
        return;
    }

    for (Entry& entry : entries) {
        tx.attachRaw(&entry.original, entry.replacement);
    }

    if (tx.commit() != NO_ERROR) { assert(false && "lua_swap: the entry hooks failed, Wow.exe's own VM stays"); }
}
}  // namespace

void lua_swap::initialize(hookkit::HookTransaction&) {
    extensions::console::kCvarRegistry->add<int>({.name = "luaSwap", .init = 0, .min{0}, .max{1}});
    extensions::console::kOnCvarsLoaded->add([] {
        if (extensions::console::kCvarRegistry->get<"luaSwap", int>() != 0) { swapVm(); }
    });
}
