#include "Item.h"

#include "Extensions.h"

#include "include/DB/ClientDB.h"
#include "include/DB/DBItemCache.h"
#include "include/DB/DBRecords.h"
#include "include/Item/CGItem_C.h"
#include "include/Lib/Lua.h"

namespace {
const char* getItemClassName(uint32_t class_id) {
    if (const auto* rec = client_db::getRowById<ItemClassRec>("itemClass", class_id)) { return rec->class_name_lang; }
    return "";
}

const char* getItemSubClassName(uint32_t class_id, uint32_t sub_class_id) {
    const auto* rec = client_db::findRow<ItemSubClassRec>("itemSubClass",
        [=](const ItemSubClassRec& row) { return row.class_id == class_id && row.sub_class_id == sub_class_id; });
    if (rec == nullptr) { return ""; }
    if (rec->verbose_name_lang != nullptr && *rec->verbose_name_lang != '\0') { return rec->verbose_name_lang; }
    return rec->display_name_lang;
}

int luaGetItemInfoInstant(LuaState* l) {
    uintptr_t record_ptr = 0;
    uint32_t item_id = 0;

    if (!lua::isNumber(l, 1) && !lua::isString(l, 1)) {
        lua::throwError(l, R"(Usage: %s(itemID | "itemName" | "itemLink"))");
    }
    if (lua::isNumber(l, 1)) {
        LuaNumber n = lua::checkNumber(l, 1);
        if (n <= 0 || n != static_cast<LuaNumber>(static_cast<uint32_t>(n))) { return 0; }
        item_id = static_cast<uint32_t>(n);
    } else if (lua::isString(l, 1)) {
        const char* input = lua::checkString(l, 1);
        item_id = CGItem_C::getItemInfoByName_hook{}(input);
        if (item_id == 0) { item_id = CGItem_C::getItemId(input); }
    }
    if (item_id != 0) { record_ptr = DbItemCache::kWdbCacheItem->getItemInfoBlockById(item_id, nullptr, 0, 0, 0); }
    if (record_ptr == 0) { lua::throwError(l, "Invalid itemID"); }

    const auto* item = reinterpret_cast<ItemCacheRec*>(record_ptr);
    lua::pushNumber(l, item_id);
    lua::pushString(l, getItemClassName(item->item_class));
    lua::pushString(l, getItemSubClassName(item->item_class, item->sub_class));
    lua::pushString(l, CGItem_C::kIdToStr[item->inv_type]);
    lua::pushString(l, CGItem_C::getInvArtById_hook{}(item->display_id));
    lua::pushNumber(l, item->item_class);
    lua::pushNumber(l, item->sub_class);

    return 7;
}

int luaOpenItem(LuaState* l) {
    lua::pushCFunction(l, luaGetItemInfoInstant);
    lua::setGlobal(l, "GetItemInfoInstant");
    return 0;
}
}  // namespace

void item::initialize(hookkit::HookTransaction&) { extensions::console::kLuaLibRegistry->add(luaOpenItem); }
