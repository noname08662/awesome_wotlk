#include "Item.h"
#include "Hooks.h"
#include "GameClient.h"
#include <string>

namespace {
const char* GetItemClassName(uint32_t classId) {
	if (auto* rec = ClientDB::GetRowById<ItemClassRec>("itemClass", classId)) {
		return rec->m_className_lang;
	}
	return "";
}

const char* GetItemSubClassName(uint32_t classId, uint32_t subClassId) {
	auto* rec = ClientDB::FindRow<ItemSubClassRec>("itemSubClass", [=](const ItemSubClassRec* row) {
		return row->m_classID == classId && row->m_subClassID == subClassId;
		});

	if (rec) {
		if (rec->m_verboseName_lang && *rec->m_verboseName_lang) {
			return rec->m_verboseName_lang;
		}
		return rec->m_displayName_lang;
	}
	return "";
}

int lua_GetItemInfoInstant(lua_State* L) {
	uintptr_t recordPtr = 0;
	uint32_t itemId = 0;

	if (Lua::lua_isnumber(L, 1)) {
		lua_Number n = Lua::luaL_checknumber(L, 1);
		if (n <= 0 || n != static_cast<lua_Number>(static_cast<uint32_t>(n))) return 0;
		itemId = static_cast<uint32_t>(n);
	}
	else if (Lua::lua_isstring(L, 1)) {
		const char* input = Lua::luaL_checkstring(L, 1);
		itemId = CGItem_C::GetItemIDByNameFn(input);
		if (!itemId) itemId = CGItem_C::GetItemIdFromLink(input);
	}
	if (itemId) recordPtr = DBItemCache::WDB_CACHE_ITEM->GetItemInfoBlockById(itemId, nullptr, 0, 0, 0);
	if (!recordPtr) return 0;

	auto* item = reinterpret_cast<ItemCacheRec*>(recordPtr);
	Lua::lua_pushnumber(L, itemId);
	Lua::lua_pushstring(L, GetItemClassName(item->m_class));
	Lua::lua_pushstring(L, GetItemSubClassName(item->m_class, item->m_subClass));
	Lua::lua_pushstring(L, g_itemIdToStr[item->m_invType]);
	Lua::lua_pushstring(L, CGItem_C::GetInventoryArtFn(item->m_displayId));
	Lua::lua_pushnumber(L, item->m_class);
	Lua::lua_pushnumber(L, item->m_subClass);

	return 7;
}

int lua_openmisclib(lua_State* L) {
	Lua::lua_pushcfunction(L, lua_GetItemInfoInstant);
	Lua::lua_setglobal(L, "GetItemInfoInstant");
	return 0;
}
}

void Item::initialize() { Hooks::FrameXML::registerLuaLib(lua_openmisclib); }