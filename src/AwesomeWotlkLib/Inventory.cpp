#include "Inventory.h"

#include <iterator>

#include "Extensions.h"

#include "include/Lib/Lua.h"
#include "include/ObjectManager/CGPlayer_C.h"
#include "include/ObjectManager/Descriptors.h"
#include "include/ObjectManager/ObjectManager.h"
#include "include/ObjectManager/ObjectManagerEnums.h"

namespace {
int luaGetInventoryItemTransmog(LuaState* l) {
    if (!lua::isString(l, 1) || !lua::isNumber(l, 2)) { lua::throwError(l, "Usage: %s(unitID, slotID)"); }
    const char* uint_id = lua::checkString(l, 1);
    LuaNumber raw = lua::checkNumber(l, 2);
    int idx = static_cast<int>(raw) - 1;
    guid_t guid = object_mgr::string2Guid(uint_id);
    if (raw != static_cast<LuaNumber>(static_cast<int>(raw)) || (guid == 0u)) {
        lua::throwError(l, "Usage: %s(unitID, slotID)");
    }
    auto* player = object_mgr::get<CGPlayer_C>(guid, eTypemaskPlayer);
    if (player == nullptr) { return 0; }
    auto* entry = player->descriptors_;
    if (entry == nullptr) { return 0; }
    if (idx < 0 || idx >= std::ssize(entry->player_entry.visible_items)) { return 0; }
    lua::pushNumber(l, entry->player_entry.visible_items[idx].entry_id);
    lua::pushNumber(l, entry->player_entry.visible_items[idx].enchant);
    return 2;
}

int luaOpenInventory(LuaState* l) {
    lua::pushCFunction(l, luaGetInventoryItemTransmog);
    lua::setGlobal(l, "GetInventoryItemTransmog");
    return 0;
}
}  // namespace

void inventory::initialize(hookkit::HookTransaction&) { extensions::console::kLuaLibRegistry->add(luaOpenInventory); }
