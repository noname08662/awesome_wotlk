#include "Spell.h"
#include "Lua.h"
#include "Hooks.h"

#undef min
#undef max

namespace {
    constexpr uint8_t OFFSET_SHAPESHIFT_FORM = 3;

    using SpellCast_t = int(__cdecl*)(int spellId, int a2, int a3, int a4, int a5);
    auto Spell_OnCastOriginal = reinterpret_cast<SpellCast_t>(0x0080DA40);

    CVar* s_cvar_enableStancePatch;

    uintptr_t spellTablePtr = ClientDB::GetDbcTable(0x00000194);

    int lua_GetSpellBaseCooldown(lua_State* L) {
        SpellRec spellData;
        uint32_t spellId = static_cast<uint32_t>(Lua::luaL_checknumber(L, 1));
        auto* header = reinterpret_cast<void*>(spellTablePtr - 0x18);

        if (!ClientDB::GetLocalizedRow(header, spellId, &spellData)) return 0;

        uint32_t cdTime = spellData.m_recoveryTime ? spellData.m_recoveryTime : spellData.m_categoryRecoveryTime;
        uint32_t gcdTime = spellData.m_startRecoveryTime;

        if (cdTime == 0) {
            for (uint32_t triggeredId : spellData.m_effectTriggerSpell) {
                if (triggeredId == 0 || triggeredId == spellId) continue;
                SpellRec triggeredData;
                if (ClientDB::GetLocalizedRow(header, triggeredId, &triggeredData)) {
                    uint32_t trigCd = triggeredData.m_recoveryTime ? triggeredData.m_recoveryTime : triggeredData.m_categoryRecoveryTime;
                    cdTime = std::max(cdTime, trigCd);
                    gcdTime = std::max(gcdTime, triggeredData.m_startRecoveryTime);
                }
            }
        }

        Lua::lua_pushnumber(L,cdTime);
        Lua::lua_pushnumber(L, gcdTime);
        return 2;
    }

    int __cdecl Spell_OnCastHook(int spellId, int a2, int a3, int a4, int a5) {
        int result = Spell_OnCastOriginal(spellId, a2, a3, a4, a5);
        if (result && Spell::IsForm(spellId)) {
            if (CGUnit_C* player = ObjectMgr::Get<CGPlayer_C>(ObjectMgr::GetPlayerGuid(), TYPEMASK_PLAYER)) {
                if (auto maybeForm = Spell::GetFormFromSpell(spellId)) {
                    player->SetValueBytes(UNIT_FIELD_BYTES_2, OFFSET_SHAPESHIFT_FORM, *maybeForm);
                }
            }
        }
        return result;
    }

    int lua_openmisclib(lua_State* L) {
        Lua::lua_pushcfunction(L, lua_GetSpellBaseCooldown);
        Lua::lua_setglobal(L, "GetSpellBaseCooldown");
        return 0;
    }

    int CVarHandler_enableStancePatch(CVar*, const char*, const char* value, void*) {
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        if (std::atoi(value) == 0) Hooks::Detach(&Spell_OnCastOriginal, Spell_OnCastHook);
        else Hooks::Detour(&Spell_OnCastOriginal, Spell_OnCastHook);
        DetourTransactionCommit();
        return 1;
    }
}

void Spell::initialize() {
    Hooks::FrameXML::registerLuaLib(lua_openmisclib);
    Hooks::FrameXML::registerCVar(&s_cvar_enableStancePatch, "enableStancePatch", nullptr, "0", CVarHandler_enableStancePatch);
}