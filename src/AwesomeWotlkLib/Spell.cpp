#include "Spell.h"

#include <algorithm>
#include <array>

#include "Extensions.h"

#include "include/DB/ClientDB.h"
#include "include/DB/DBRecords.h"
#include "include/Lib/Lua.h"

namespace {
int luaGetSpellBaseCooldown(LuaState* l) {
    if (!lua::isNumber(l, 1)) { lua::throwError(l, "Usage: %s(spellID)"); }
    auto spell_id = static_cast<uint32_t>(lua::checkNumber(l, 1));

    const auto* spell_data = client_db::getRowById<SpellRec>("spell", spell_id);
    if (spell_data == nullptr) { return 0; }

    uint32_t cd_time = spell_data->recovery_time != 0 ? spell_data->recovery_time : spell_data->category_recovery_time;
    uint32_t gcd_time = spell_data->start_recovery_time;

    if (cd_time == 0) {
        for (uint32_t triggered_id : spell_data->effect_trigger_spell) {
            if (triggered_id == 0 || triggered_id == spell_id) { continue; }
            if (const auto* rec = client_db::getRowById<SpellRec>("spell", triggered_id)) {
                uint32_t trig_cd = rec->recovery_time != 0 ? rec->recovery_time : rec->category_recovery_time;
                cd_time = std::max(cd_time, trig_cd);
                gcd_time = std::max(gcd_time, rec->start_recovery_time);
            }
        }
    }

    lua::pushNumber(l, cd_time);
    lua::pushNumber(l, gcd_time);
    return 2;
}

int luaOpenSpell(LuaState* l) {
    lua::pushCFunction(l, luaGetSpellBaseCooldown);
    lua::setGlobal(l, "GetSpellBaseCooldown");
    return 0;
}
}  // namespace

void spell::initialize(hookkit::HookTransaction&) { extensions::console::kLuaLibRegistry->add(luaOpenSpell); }
