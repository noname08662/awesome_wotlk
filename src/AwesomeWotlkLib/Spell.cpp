#include "Spell.h"
#include "Lua.h"
#include "Hooks.h"

#undef min
#undef max

namespace {
int lua_GetSpellBaseCooldown(lua_State* L) {
	uint32_t spellId = static_cast<uint32_t>(Lua::luaL_checknumber(L, 1));

	auto* spellData = ClientDB::GetRowById<SpellRec>("spell", spellId);
	if (!spellData) return 0;

	uint32_t cdTime = spellData->m_recoveryTime ? spellData->m_recoveryTime : spellData->m_categoryRecoveryTime;
	uint32_t gcdTime = spellData->m_startRecoveryTime;

	if (cdTime == 0) {
		for (uint32_t triggeredId : spellData->m_effectTriggerSpell) {
			if (triggeredId == 0 || triggeredId == spellId) continue;
			if (auto* rec = ClientDB::GetRowById<SpellRec>("spell", triggeredId)) {
				uint32_t trigCd = rec->m_recoveryTime ? rec->m_recoveryTime : rec->m_categoryRecoveryTime;
				cdTime = std::max(cdTime, trigCd);
				gcdTime = std::max(gcdTime, rec->m_startRecoveryTime);
			}
		}
	}

	Lua::lua_pushnumber(L, cdTime);
	Lua::lua_pushnumber(L, gcdTime);
	return 2;
}

int lua_openmisclib(lua_State* L) {
	Lua::lua_pushcfunction(L, lua_GetSpellBaseCooldown);
	Lua::lua_setglobal(L, "GetSpellBaseCooldown");
	return 0;
}
}

void Spell::initialize() {
	Hooks::FrameXML::registerLuaLib(lua_openmisclib);
}
