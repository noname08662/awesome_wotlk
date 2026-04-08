#include "Inventory.h"
#include "Lua.h"
#include "Hooks.h"
#include "GameClient.h"

namespace {
    int lua_GetInventoryItemTransmog(lua_State* L) {
        const char* playerKey = Lua::luaL_checkstring(L, 1);
        if (!playerKey) return 0;
        lua_Number raw = Lua::luaL_checknumber(L, 2);
        int idx = static_cast<int>(raw) - 1;
        if (raw != static_cast<lua_Number>(static_cast<int>(raw))) return 0;
        CGPlayer_C* player = ObjectMgr::Get<CGPlayer_C>(ObjectMgr::GetPlayerGuid(), TYPEMASK_PLAYER);
        if (!player) return 0;
        if (idx < 0 || idx >= 19) return 0;
        if (PlayerEntry* entry = player->GetEntry<PlayerEntry>()) {
            Lua::lua_pushnumber(L, entry->m_visibleItems[idx].m_entryId);
            Lua::lua_pushnumber(L, entry->m_visibleItems[idx].m_enchant);
            return 2;
        }
        return 0;
    }

    int lua_openlibinventory(lua_State* L) {
        Lua::lua_pushcfunction(L, lua_GetInventoryItemTransmog);
        Lua::lua_setglobal(L, "GetInventoryItemTransmog");
        return 0;
    }
}

void Inventory::initialize() {
    Hooks::FrameXML::registerLuaLib(lua_openlibinventory);
}