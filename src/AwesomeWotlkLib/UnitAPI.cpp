#include "UnitAPI.h"

#include <array>
#include <format>
#include <string>

#include "Extensions.h"
#include "NamePlates.h"

#include "ObjectManager/CGUnit_C.h"
#include "include/Lib/Lua.h"

namespace {
bool checkToken(LuaState* l, const char* token, guid_t guid) {
    if (const guid_t guid_t = object_mgr::guidFromUnitId{}(token); guid_t == guid) {
        lua::pushString(l, token);
        return true;
    }
    return false;
}

bool checkIndexedTokens(LuaState* l, const char* base, int start, int end, guid_t guid) {
    for (int i = start; i <= end; ++i) {
        std::string token = std::format("{}{}", base, i);
        if (checkToken(l, token.c_str(), guid)) { return true; }
    }
    return false;
}

int unitHasFlag(LuaState* l, uint32_t flag) {
    if (!lua::isString(l, 1)) { lua::throwError(l, "Usage: %s(unitID)"); }
    auto* unit = object_mgr::get<CGUnit_C>(object_mgr::guidFromUnitId{}(lua::checkString(l, 1)), eTypemaskUnit);
    if ((unit != nullptr) && ((unit->descriptors_->flags & flag) != 0u)) {
        lua::pushNumber(l, 1);
        return 1;
    }
    return 0;
}
}  // namespace

namespace {
int luaUnitIsControlled(LuaState* l) {
    return unitHasFlag(l, eUnitFlagFleeing | eUnitFlagConfused | eUnitFlagStunned | eUnitFlagPacified);
}

int luaUnitIsDisarmed(LuaState* l) { return unitHasFlag(l, eUnitFlagDisarmed); }

int luaUnitIsSilenced(LuaState* l) { return unitHasFlag(l, eUnitFlagSilenced); }

int luaUnitOccupations(LuaState* l) {
    if (!lua::isString(l, 1)) { lua::throwError(l, "Usage: %s(unitID)"); }
    auto* unit = object_mgr::get<CGUnit_C>(object_mgr::guidFromUnitId{}(lua::checkString(l, 1)), eTypemaskUnit);
    if (unit == nullptr) { return 0; }
    lua::pushNumber(l, unit->descriptors_->npc_flags);
    return 1;
}

int luaUnitOwner(LuaState* l) {
    if (!lua::isString(l, 1)) { lua::throwError(l, "Usage: %s(unitID)"); }
    auto* unit = object_mgr::get<CGUnit_C>(object_mgr::guidFromUnitId{}(lua::checkString(l, 1)), eTypemaskUnit);
    if (unit == nullptr) { return 0; }

    UnitEntry* desc = unit->descriptors_;
    guid_t owner_guid = (desc->summoned_by != 0u) ? desc->summoned_by : desc->created_by;
    if (owner_guid == 0u) { return 0; }

    auto* owner = object_mgr::get<CGUnit_C>(owner_guid, eTypemaskUnit);
    if (owner == nullptr) { return 0; }

    const char* name = owner->getObjectName();
    lua::pushString(l, (name != nullptr) ? name : "UNKNOWN");

    std::string guid_str = std::format("0x{:016X}", owner_guid);
    lua::pushString(l, guid_str.c_str());
    return 2;
}

int luaUnitTokenFromGuid(LuaState* l) {
    if (!lua::isString(l, 1)) { lua::throwError(l, "Usage: %s(GUID)"); }

    guid_t guid = object_mgr::str2Guid{}(lua::checkString(l, 1));
    if ((guid == 0u) || ((object_mgr::get<CGUnit_C>(guid, eTypemaskUnit)) == nullptr)) { return 0; }

    for (const char* token : {"player", "vehicle", "pet", "target", "focus", "mouseover"}) {
        if (checkToken(l, token, guid)) { return 1; }
    }

    if (checkIndexedTokens(l, "party", 1, 4, guid)) { return 1; }
    if (checkIndexedTokens(l, "partypet", 1, 4, guid)) { return 1; }
    if (checkIndexedTokens(l, "raid", 1, 40, guid)) { return 1; }
    if (checkIndexedTokens(l, "raidpet", 1, 40, guid)) { return 1; }
    if (checkIndexedTokens(l, "arena", 1, 5, guid)) { return 1; }
    if (checkIndexedTokens(l, "arenapet", 1, 5, guid)) { return 1; }
    if (checkIndexedTokens(l, "boss", 1, 5, guid)) { return 1; }

    int token_id = name_plates::getTokenId(guid);
    if (token_id >= 0) {
        std::string token = std::format("nameplate{}", token_id + 1);
        lua::pushString(l, token.c_str());
        return 1;
    }
    return 0;
}

int luaOpenUnit(LuaState* l) {
    static constexpr std::array<lua::LuaLReg, 6> kFuncs = {{
        {.name = "UnitIsControlled", .func = luaUnitIsControlled},
        {.name = "UnitIsDisarmed", .func = luaUnitIsDisarmed},
        {.name = "UnitIsSilenced", .func = luaUnitIsSilenced},
        {.name = "UnitOccupations", .func = luaUnitOccupations},
        {.name = "UnitOwner", .func = luaUnitOwner},
        {.name = "UnitTokenFromGUID", .func = luaUnitTokenFromGuid},
    }};
    for (const auto& [name, func] : kFuncs) {
        lua::pushCFunction(l, func);
        lua::setGlobal(l, name);
    }
    return 0;
}
}  // namespace

void unit_api::initialize(hookkit::HookTransaction&) { extensions::console::kLuaLibRegistry->add(luaOpenUnit); }
