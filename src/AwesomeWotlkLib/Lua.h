#pragma once
#include "GameClient.h"
#include <string>
#include <ranges>

namespace Lua {
    inline lua_State* GetLuaState() { return reinterpret_cast<lua_State* (*)()>(0x00817DB0)(); }
    inline int GetLuaRefErrorHandler() { return *reinterpret_cast<int*>(0x00AF576C); }

    using lua_CFunction = int(*)(lua_State*);
    struct luaL_Reg_t {
        const char* name;
        lua_CFunction func;
    };
    using luaL_Reg = luaL_Reg_t;

    inline constexpr int T_NONE = -1;
    inline constexpr int T_NIL = 0;
    inline constexpr int T_BOOLEAN = 1;
    inline constexpr int T_LIGHTUSERDATA = 2;
    inline constexpr int T_NUMBER = 3;
    inline constexpr int T_STRING = 4;
    inline constexpr int T_TABLE = 5;
    inline constexpr int T_FUNCTION = 6;
    inline constexpr int T_USERDATA = 7;
    inline constexpr int T_THREAD = 8;

    inline constexpr int REGISTRYINDEX = -10000;
    inline constexpr int ENVIRONINDEX = -10001;
    inline constexpr int GLOBALSINDEX = -10002;
    inline constexpr int upvalueindex(int i) { return GLOBALSINDEX - i; }

    using fn_luaL_checktype = void(*)(lua_State*, int, int);
    using fn_luaL_checklstring = const char* (*)(lua_State*, int, size_t*);
    using fn_luaL_checknumber = lua_Number(*)(lua_State*, int);
    using fn_lua_touserdata = void* (*)(lua_State*, int);
    using fn_lua_tonumber = double(*)(lua_State*, int);
    using fn_lua_tolstring = const char* (*)(lua_State*, int, size_t*);
    using fn_lua_toframe = CSimpleFrame* (*)(lua_State*, int);
    using fn_lua_toboolean = bool(*)(lua_State*, int);
    using fn_lua_pushstring = void(*)(lua_State*, const char*);
    using fn_lua_pushboolean = void(*)(lua_State*, bool);
    using fn_lua_pushvalue = void(*)(lua_State*, int);
    using fn_lua_pushnumber = void(*)(lua_State*, lua_Number);
    using fn_lua_pushlightuserdata = void(*)(lua_State*, void*);
    using fn_lua_pushcclosure = void(*)(lua_State*, lua_CFunction, int);
    using fn_lua_pushnil = void(*)(lua_State*);
    using fn_lua_rawseti = void(*)(lua_State*, int, int);
    using fn_lua_rawgeti = void(*)(lua_State*, int, int);
    using fn_lua_rawset = void(*)(lua_State*, int);
    using fn_lua_rawget = void(*)(lua_State*, int);
    using fn_lua_setfield = void(*)(lua_State*, int, const char*);
    using fn_lua_getfield = void(*)(lua_State*, int, const char*);
    using fn_lua_next = int(*)(lua_State*, int);
    using fn_lua_insert = void(*)(lua_State*, int);
    using fn_lua_gettop = int(*)(lua_State*);
    using fn_lua_settop = void(*)(lua_State*, int);
    using fn_lua_objlen = int(*)(lua_State*, int);
    using fn_lua_type = int(*)(lua_State*, int);
    using fn_lua_pcall = int(*)(lua_State*, int, int, int);
    using fn_lua_GetParamValue = int(*)(lua_State*, int, int);
    using fn_lua_createtable = void(*)(lua_State*, int, int);
    using fn_lua_newuserdata = void* (*)(lua_State*, size_t);
    using fn_lua_setmetatable = int(*)(lua_State*, int);

    inline auto OpenFrameXMLApiFn = reinterpret_cast<DummyCallback_t>(0x00530F85);

    inline void luaL_checktype(lua_State* L, int idx, int t) {
        reinterpret_cast<fn_luaL_checktype>(0x0084F960)(L, idx, t);
    }
    inline const char* luaL_checklstring(lua_State* L, int idx, size_t* len) {
        return reinterpret_cast<fn_luaL_checklstring>(0x0084F9F0)(L, idx, len);
    }
    inline lua_Number luaL_checknumber(lua_State* L, int idx) {
        return reinterpret_cast<fn_luaL_checknumber>(0x0084FAB0)(L, idx);
    }
    inline void* lua_touserdata(lua_State* L, int idx) {
        return reinterpret_cast<fn_lua_touserdata>(0x0084E1C0)(L, idx);
    }
    inline double lua_tonumber(lua_State* L, int n_param) {
        return reinterpret_cast<fn_lua_tonumber>(0x0084E030)(L, n_param);
    }
    inline const char* lua_tolstring(lua_State* L, int idx, size_t* len) {
        return reinterpret_cast<fn_lua_tolstring>(0x0084E0E0)(L, idx, len);
    }
    inline bool lua_toboolean(lua_State* L, int idx) {
        return reinterpret_cast<fn_lua_toboolean>(0x0084E0B0)(L, idx);
    }
    inline void lua_pushstring(lua_State* L, const char* str) {
        reinterpret_cast<fn_lua_pushstring>(0x0084E350)(L, str);
    }
    inline void lua_pushboolean(lua_State* L, bool b) {
        reinterpret_cast<fn_lua_pushboolean>(0x0084E4D0)(L, b);
    }
    inline void lua_pushvalue(lua_State* L, int idx) {
        reinterpret_cast<fn_lua_pushvalue>(0x0084DE50)(L, idx);
    }
    inline void lua_pushnumber(lua_State* L, lua_Number v) {
        reinterpret_cast<fn_lua_pushnumber>(0x0084E2A0)(L, v);
    }
    inline void lua_pushlightuserdata(lua_State* L, void* data) {
        reinterpret_cast<fn_lua_pushlightuserdata>(0x0084E500)(L, data);
    }
    inline void lua_pushcclosure(lua_State* L, lua_CFunction func, int c) {
        reinterpret_cast<fn_lua_pushcclosure>(0x0084E400)(L, func, c);
    }
    inline void lua_pushnil(lua_State* L) {
        reinterpret_cast<fn_lua_pushnil>(0x0084E280)(L);
    }
    inline void lua_rawseti(lua_State* L, int idx, int pos) {
        reinterpret_cast<fn_lua_rawseti>(0x0084EA00)(L, idx, pos);
    }
    inline void lua_rawgeti(lua_State* L, int idx, int pos) {
        reinterpret_cast<fn_lua_rawgeti>(0x0084E670)(L, idx, pos);
    }
    inline void lua_rawset(lua_State* L, int idx) {
        reinterpret_cast<fn_lua_rawset>(0x0084E970)(L, idx);
    }
    inline void lua_rawget(lua_State* L, int idx) {
        reinterpret_cast<fn_lua_rawget>(0x0084E600)(L, idx);
    }
    inline void lua_setfield(lua_State* L, int idx, const char* str) {
        reinterpret_cast<fn_lua_setfield>(0x0084E900)(L, idx, str);
    }
    inline void lua_getfield(lua_State* L, int idx, const char* str) {
        reinterpret_cast<fn_lua_getfield>(0x0084E590)(L, idx, str);
    }
    inline int lua_next(lua_State* L, int idx) {
        return reinterpret_cast<fn_lua_next>(0x0084EF50)(L, idx);
    }
    inline void lua_insert(lua_State* L, int idx) {
        reinterpret_cast<fn_lua_insert>(0x0084DCC0)(L, idx);
    }
    inline int lua_gettop(lua_State* L) {
        return reinterpret_cast<fn_lua_gettop>(0x0084DBD0)(L);
    }
    inline void lua_settop(lua_State* L, int idx) {
        reinterpret_cast<fn_lua_settop>(0x0084DBF0)(L, idx);
    }
    inline int lua_objlen(lua_State* L, int idx) {
        return reinterpret_cast<fn_lua_objlen>(0x0084E150)(L, idx);
    }
    inline int lua_type(lua_State* L, int idx) {
        return reinterpret_cast<fn_lua_type>(0x0084DEB0)(L, idx);
    }
    inline int lua_pcall(lua_State* L, int argn, int retn, int eh) {
        return reinterpret_cast<fn_lua_pcall>(0x0084EC50)(L, argn, retn, eh);
    }
    inline int lua_GetParamValue(lua_State* L, int idx, int default_) {
        return reinterpret_cast<fn_lua_GetParamValue>(0x00815500)(L, idx, default_);
    }
    inline void lua_createtable(lua_State* L, int narr, int nrec) {
        reinterpret_cast<fn_lua_createtable>(0x0084E6E0)(L, narr, nrec);
    }
    inline void* lua_newuserdata(lua_State* L, size_t size) {
        return reinterpret_cast<fn_lua_newuserdata>(0x0084F0F0)(L, size);
    }
    inline int lua_setmetatable(lua_State* L, int idx) {
        return reinterpret_cast<fn_lua_setmetatable>(0x0084EA90)(L, idx);
    }
    inline CSimpleFrame* lua_toframe(lua_State* L, int idx) {
        return reinterpret_cast<fn_lua_toframe>(0x004A81B0)(L, idx);
    }
    inline void lua_pushframe(lua_State* L, CSimpleFrame* frame) {
        lua_rawgeti(L, REGISTRYINDEX, frame->GetRefTable());
    }

    inline void lua_pop(lua_State* L, int n) { lua_settop(L, -(n)-1); }
    inline void lua_newtable(lua_State* L) { lua_createtable(L, 0, 0); }
    inline void lua_pushcfunction(lua_State* L, lua_CFunction f) { lua_pushcclosure(L, f, 0); }

    inline bool lua_isfunction(lua_State* L, int n) { return lua_type(L, n) == T_FUNCTION; }
    inline bool lua_istable(lua_State* L, int n) { return lua_type(L, n) == T_TABLE; }
    inline bool lua_islightuserdata(lua_State* L, int n) { return lua_type(L, n) == T_LIGHTUSERDATA; }
    inline bool lua_isnil(lua_State* L, int n) { return lua_type(L, n) == T_NIL; }
    inline bool lua_isstring(lua_State* L, int n) { return lua_type(L, n) == T_STRING; }
    inline bool lua_isnumber(lua_State* L, int n) { return lua_type(L, n) == T_NUMBER; }
    inline bool lua_isboolean(lua_State* L, int n) { return lua_type(L, n) == T_BOOLEAN; }
    inline bool lua_isthread(lua_State* L, int n) { return lua_type(L, n) == T_THREAD; }
    inline bool lua_isnone(lua_State* L, int n) { return lua_type(L, n) == T_NONE; }

    inline bool lua_isuserdata(lua_State* L, int n) {
        int t = lua_type(L, n);
        return (t == T_USERDATA || t == T_LIGHTUSERDATA);
    }
    inline bool lua_isnoneornil(lua_State* L, int n) {
        int t = lua_type(L, n);
        return (t == T_NONE || t == T_NIL);
    }

    inline void lua_setglobal(lua_State* L, const char* s) { lua_setfield(L, GLOBALSINDEX, s); }
    inline void lua_getglobal(lua_State* L, const char* s) { lua_getfield(L, GLOBALSINDEX, s); }

    inline const char* lua_tostring(lua_State* L, int i) { return lua_tolstring(L, i, nullptr); }
    inline const char* luaL_checkstring(lua_State* L, int i) { return luaL_checklstring(L, i, nullptr); }

    inline CSimpleFrame* lua_toframe_silent(lua_State* L, int idx) {
        lua_rawgeti(L, idx, 0);
        CSimpleFrame* frame = static_cast<CSimpleFrame*>(lua_touserdata(L, -1));
        lua_pop(L, 1);
        return frame;
    }

    inline void lua_wipe(lua_State* L, int idx) {
        if (idx < 0) idx = lua_gettop(L) + idx + 1;
        lua_pushnil(L);             // push first key
        while (lua_next(L, idx)) {  // pushes key, value
            lua_pop(L, 1);          // pop value, leave key
            lua_pushnil(L);         // push key, nil
            lua_rawset(L, idx);     // table[key] = nil (pops key and nil)
            lua_pushnil(L);         // prepare next iteration (push nil as key)
        }
    }

    inline void lua_pushguid(lua_State* L, guid_t guid) {
        char buf[24];
        ObjectMgr::Guid2HexString(guid, buf);
        lua_pushstring(L, buf);
    }

    struct MockCStatus {
        void** vtable;
    };

    inline bool CheckIfBindingOrHeaderExists(const char* upperName, const char* upperHeader) {
        lua_State* L = GetLuaState();
        if (!L) return false;

        if (upperName && upperName[0]) {
            std::string key = "BINDING_NAME_" + std::string(upperName);
            lua_getfield(L, GLOBALSINDEX, key.c_str());
            if (!lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return true;
            }
            lua_pop(L, 1);
        }
        if (upperHeader && upperHeader[0]) {
            std::string key = "BINDING_HEADER_" + std::string(upperHeader);
            lua_getfield(L, GLOBALSINDEX, key.c_str());
            if (!lua_isnil(L, -1)) {
                lua_pop(L, 1);
                return true;
            }
            lua_pop(L, 1);
        }

        void** hashTable = *reinterpret_cast<void***>(0x00BEADD8) + 3;
        auto SStrHashHT = reinterpret_cast<void* (__thiscall*)(void*, const char*)>(0x0055F4D0);

        if (upperName && upperName[0] && SStrHashHT(hashTable, upperName)) return true;
        if (upperHeader && upperHeader[0]) {
            std::string fullHeaderKey = "HEADER_" + std::string(upperHeader);
            if (SStrHashHT(hashTable, fullHeaderKey.c_str())) return true;
        }
        return false;
    }

    inline bool CheckSlashCommandExists(const char* cmdKey) {
        lua_State* L = GetLuaState();
        if (!L || !cmdKey) return false;

        Lua::lua_getglobal(L, "SlashCmdList");
        if (Lua::lua_istable(L, -1)) {
            Lua::lua_getfield(L, -1, cmdKey);
            if (!Lua::lua_isnil(L, -1)) {
                Lua::lua_pop(L, 2);
                return true;
            }
            Lua::lua_pop(L, 1);
        }
        Lua::lua_pop(L, 1);

        std::string globalVar = "SLASH_" + std::string(cmdKey) + "1";
        Lua::lua_getglobal(L, globalVar.c_str());
        bool exists = !Lua::lua_isnil(L, -1);
        Lua::lua_pop(L, 1);
        return exists;
    }

    inline void RegisterSlashCommand(const char* cmdKey, const char* slashStr, lua_CFunction func) {
        lua_State* L = GetLuaState();
        if (!L || !cmdKey || !slashStr) return;

        if (CheckSlashCommandExists(cmdKey)) return;

        Lua::lua_pushcfunction(L, func);
        Lua::lua_getglobal(L, "SlashCmdList");
        if (Lua::lua_istable(L, -1)) {
            Lua::lua_pushvalue(L, -2);
            Lua::lua_setfield(L, -2, cmdKey);
        }
        Lua::lua_pop(L, 2);

        std::string globalVar = "SLASH_" + std::string(cmdKey) + "1";
        Lua::lua_pushstring(L, slashStr);
        Lua::lua_setglobal(L, globalVar.c_str());
    }

    inline void RegisterLuaBinding(const char* bindsSet, const char* bindingName, const char* bindingText, const char* bindingHeaderName, const char* bindingHeaderText, const char* luaScript) {
        lua_State* L = GetLuaState();
        if (!L) return;

        std::string upperHeader = bindingHeaderName ? bindingHeaderName : "";
        std::ranges::transform(upperHeader, upperHeader.begin(),
            [](unsigned char c) { return std::toupper(c); });

        std::string upperName = bindingName ? bindingName : "";
        std::ranges::transform(upperName, upperName.begin(),
            [](unsigned char c) { return std::toupper(c); });

        if (CheckIfBindingOrHeaderExists(upperName.c_str(), upperHeader.c_str())) return;

        if (!upperHeader.empty()) {
            lua_pushstring(L, bindingHeaderText ? bindingHeaderText : "");
            std::string key = "BINDING_HEADER_" + upperHeader;
            lua_setfield(L, GLOBALSINDEX, key.c_str());
        }
        if (!upperName.empty()) {
            lua_pushstring(L, bindingText ? bindingText : "");
            std::string key = "BINDING_NAME_" + upperName;
            lua_setfield(L, GLOBALSINDEX, key.c_str());
        }

        XMLObject node(0, "Bindings");
        node.setValue("name", upperName.c_str());
        node.setValue("header", bindingHeaderName ? bindingHeaderName : "");

        *reinterpret_cast<char**>(reinterpret_cast<uint8_t*>(&node) + 0x18) = const_cast<char*>(luaScript);

        static void* dummyVtable[4] = { nullptr, nullptr, nullptr, nullptr };
        MockCStatus status;
        status.vtable = dummyVtable;

        auto LoadBinding = reinterpret_cast<void(__thiscall*)(void*, const char*, XMLObject*, MockCStatus*)>(0x00564470);
        if (void* pEngine = *reinterpret_cast<void**>(0x00BEADD8)) LoadBinding(pEngine, bindsSet, &node, &status);
    }
}