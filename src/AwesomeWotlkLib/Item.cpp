#include "Item.h"
#include "Hooks.h"
#include "GameClient.h"
#include <string>

namespace {
    uint32_t extractItemId(const char* hyperlink) {
        if (!hyperlink) return 0;
        const char* itemPos = strstr(hyperlink, "|Hitem:");
        if (!itemPos) itemPos = strstr(hyperlink, "|hitem:");
        if (!itemPos) return 0;
        itemPos += 7; // Skip past "|Hitem:"
        char* endPtr;
        uint32_t itemId = strtoul(itemPos, &endPtr, 10);
        return (endPtr != itemPos && (*endPtr == ':' || *endPtr == '|')) ? itemId : 0;
    }

    uintptr_t classTablePtr = ClientDB::GetDbcTable(0x00000147);
    const char* GetItemClassName(uint32_t classId) {
        auto* header = reinterpret_cast<DBCHeader*>(classTablePtr - 0x18);
        uint8_t rowBuffer[680];
        if (ClientDB::GetLocalizedRow(header, classId, rowBuffer)) {
            return reinterpret_cast<const char*>(reinterpret_cast<ItemClassRec*>(rowBuffer)->m_className_lang);
        }
        return "";
    }

    uintptr_t subClassTablePtr = ClientDB::GetDbcTable(0x00000152);
    const char* GetItemSubClassName(uint32_t classId, uint32_t subClassId) {
        auto* header = reinterpret_cast<DBCHeader*>(subClassTablePtr - 0x18);
        uint8_t rowBuffer[680];
        for (uint32_t i = 0; i <= header->m_maxIndex; ++i) {
            if (ClientDB::GetLocalizedRow(header, i, rowBuffer)) {
                auto* subclass = reinterpret_cast<ItemSubClassRec*>(rowBuffer);
                if (subclass->m_classID == classId && subclass->m_subClassID == subClassId) {
                    return reinterpret_cast<const char*>(subclass->m_displayName_lang);
                }
            }
        }
        return "";
    }

    uintptr_t displayInfoTablePtr = ClientDB::GetDbcTable(0x00000149);
    std::string GetItemIcon(uint32_t displayId) {
        auto* header = reinterpret_cast<DBCHeader*>(displayInfoTablePtr - 0x18);
        uint8_t rowBuffer[680];
        if (ClientDB::GetLocalizedRow(header, displayId, rowBuffer)) {
            auto* itemData = reinterpret_cast<ItemDisplayInfoRec*>(rowBuffer);
            const char* iconName = reinterpret_cast<const char*>(itemData->m_inventoryIcon);
            if (iconName && iconName[0] != '\0') return std::string("Interface\\Icons\\") + iconName;
        }
        return {};
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
            itemId = CGGameUI::GetItemIDByNameFn(input);
            if (!itemId) itemId = extractItemId(input);
        }
        if (itemId) recordPtr = DBItemCache::WDB_CACHE_ITEM->GetItemInfoBlockById(itemId, nullptr, 0, 0, 0);
        if (!recordPtr) return 0;

        auto* item = reinterpret_cast<ItemCacheRec*>(recordPtr);
        Lua::lua_pushnumber(L, itemId);
        Lua::lua_pushstring(L, GetItemClassName(item->m_class));
        Lua::lua_pushstring(L, GetItemSubClassName(item->m_class, item->m_subClass));
        Lua::lua_pushstring(L, idToStr[item->m_invType]);
        Lua::lua_pushstring(L, GetItemIcon(item->m_displayId).c_str());
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

void Item::initialize() {
    Hooks::FrameXML::registerLuaLib(lua_openmisclib);
}