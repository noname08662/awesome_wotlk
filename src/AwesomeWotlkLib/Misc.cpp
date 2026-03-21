#include "Misc.h"
#include "GameClient.h"
#include "Hooks.h"
#include "Utils.h"

namespace {
    bool g_cursorKeywordActive = false;
    bool g_playerLocationKeywordActive = false;

    int g_iAngle = 0;
    int g_iMode = 0;

    CVar* s_cvar_interactionMode;
    CVar* s_cvar_interactionAngle;

    const std::vector<uint8_t> validTypes = {
            GAMEOBJECT_TYPE_DOOR, GAMEOBJECT_TYPE_BUTTON, GAMEOBJECT_TYPE_QUESTGIVER,
            GAMEOBJECT_TYPE_CHEST, GAMEOBJECT_TYPE_BINDER, GAMEOBJECT_TYPE_CHAIR,
            GAMEOBJECT_TYPE_SPELL_FOCUS, GAMEOBJECT_TYPE_GOOBER, GAMEOBJECT_TYPE_FISHINGNODE,
            GAMEOBJECT_TYPE_MAILBOX, GAMEOBJECT_TYPE_MEETINGSTONE, GAMEOBJECT_TYPE_GUILD_BANK
    };

    int lua_FlashWindow(lua_State* L) {
        if (HWND hwnd = GetGameWindow()) FlashWindow(hwnd, FALSE);
        return 0;
    }

    int lua_IsWindowFocused(lua_State* L) {
        HWND hwnd = GetGameWindow();
        if (!hwnd || GetForegroundWindow() != hwnd) return 0;
        Lua::lua_pushnumber(L, 1.0);
        return 1;
    }

    int lua_FocusWindow(lua_State* L) {
        if (HWND hwnd = GetGameWindow()) SetForegroundWindow(hwnd);
        return 0;
    }

    int lua_CopyToClipboard(lua_State* L) {
        const char* str = Lua::luaL_checkstring(L, 1);
        if (str && str[0]) CopyToClipboardU8(str, nullptr);
        return 0;
    }

    guid_t s_requestedInteraction = 0;
    void ProcessQueuedInteraction() {
        if (!s_requestedInteraction) return;
        if (CGObject_C* object = ObjectMgr::Get<CGObject_C>(s_requestedInteraction, static_cast<ETypeMask>(TYPEMASK_GAMEOBJECT | TYPEMASK_UNIT))) {
            object->OnRightClick(); // safe internal call, no Lua taint
        }
        s_requestedInteraction = 0;
    }

    bool IsInteractableGameObject(uint8_t type) {
        return std::ranges::any_of(validTypes, [type](uint8_t t) { return t == type; });
    }

    auto isValidObject = [&](CGObject_C* object, const CGUnit_C* player) -> bool {
        if (object->m_typeID == TYPEID_UNIT) {
            uint32_t dynFlags = object->GetValue<uint32_t>(UNIT_DYNAMIC_FLAGS);
            uint32_t unitFlags = object->GetValue<uint32_t>(UNIT_FIELD_FLAGS);
            uint32_t npcFlags = object->GetValue<uint32_t>(UNIT_NPC_FLAGS);

            bool isLootable = (dynFlags & UNIT_DYNFLAG_LOOTABLE) != 0;
            bool isSkinnable = (unitFlags & UNIT_FLAG_SKINNABLE) != 0;
            bool canAssist = player->CanAssist(reinterpret_cast<CGUnit_C*>(object), true);

            return isLootable || isSkinnable || (canAssist && npcFlags != 0);
        }
        if (object->m_typeID == TYPEID_GAMEOBJECT) {
            uint32_t bytes = object->GetValue<uint32_t>(GAMEOBJECT_BYTES_1);
            auto* go = object->As<CGGameObject_C>();
            return go && IsInteractableGameObject((bytes >> 8) & 0xFF) && go->CanUseNow();
        }
        return false;
        };

    int lua_QueueInteract(lua_State* L) {
        if (!IsInWorld()) return 0;

        std::string modifier;
        bool hasModifier = !Lua::lua_isnoneornil(L, 1);

        if (hasModifier) {
            const char* raw = Lua::lua_tostring(L, 1);
            if (!raw) return 0;
            std::string modStr = raw;

            for (char c : modStr) {
                if (!std::isalnum(static_cast<unsigned char>(c))) return 0;
            }
            modifier = modStr;
        }

        guid_t candidate = 0;
        float bestDistance = 3000.0f;

        CGPlayer_C* player = ObjectMgr::Get<CGPlayer_C>(ObjectMgr::GetPlayerGuid(), TYPEMASK_PLAYER);
        if (!player) return 0;

        int angleDegrees = g_iAngle / 2;
        bool lookInAngle = g_iMode == 1;

        VecXYZ posPlayer{};
        player->GetPosition(*reinterpret_cast<C3Vector*>(&posPlayer));

        auto trySetCandidate = [&](guid_t guid) {
            CGObject_C* object = ObjectMgr::Get<CGObject_C>(guid, static_cast<ETypeMask>(TYPEMASK_GAMEOBJECT | TYPEMASK_UNIT));
            if (!object) return;

            float distance = player->GetDistance(object);
            if (distance == 0.f || distance > 20.0f || distance > bestDistance) return;

            if (!isValidObject(object, player)) return;

            if (lookInAngle) {
                VecXYZ posObject{};
                object->GetPosition(*reinterpret_cast<C3Vector*>(&posObject));
                float dx = posObject.x - posPlayer.x;
                float dy = posObject.y - posPlayer.y;

                float length = sqrtf(dx * dx + dy * dy);
                if (length == 0.f) return;

                dx /= length;
                dy /= length;

                float facing = player->GetFacing();
                float fx = cosf(facing);
                float fy = sinf(facing);

                if (dx * fx + dy * fy < cosf(angleDegrees * (M_PI / 180.0f))) return;
            }

            candidate = guid;
            bestDistance = distance;
            };

        if (!hasModifier) {
            ObjectMgr::EnumObjects([&](guid_t guid) {
                if (guid != player->GetGUID()) trySetCandidate(guid);
                return true;
                });
        }
        else if (guid_t guid = ObjectMgr::GetGuidByUnitID(modifier.c_str())) {
            trySetCandidate(guid);
        }
        if (candidate != 0) s_requestedInteraction = candidate;

        return 0;
    }

    int InteractFunction_C(lua_State* L) {
        const char* param = nullptr;
        if (!Lua::lua_isnoneornil(L, 1)) {
            param = Lua::lua_tostring(L, 1);
        }

        Lua::lua_getglobal(L, "SecureCmdOptionParse");
        if (!Lua::lua_isfunction(L, -1)) {
            Lua::lua_pop(L, 1);
            Lua::lua_pushcfunction(L, lua_QueueInteract);
            if (Lua::lua_isfunction(L, -1)) {
                if (Lua::lua_pcall(L, 0, 0, 0) != 0) {
                    Lua::lua_pop(L, 1);
                }
            }
            return 0;
        }

        if (param)  Lua::lua_pushstring(L, param);
        else Lua::lua_pushnil(L);

        if (Lua::lua_pcall(L, 1, 2, 0) != 0) {
            Lua::lua_pop(L, 1);
            Lua::lua_pushcfunction(L, lua_QueueInteract);
            if (Lua::lua_isfunction(L, -1)) {
                if (Lua::lua_pcall(L, 0, 0, 0) != 0) {
                    Lua::lua_pop(L, 1);
                }
            }
            return 0;
        }

        if (!Lua::lua_isnil(L, -1)) {
            Lua::lua_pushcfunction(L, lua_QueueInteract);
            if (Lua::lua_isfunction(L, -1)) {
                Lua::lua_pushvalue(L, -2);
                if (Lua::lua_pcall(L, 1, 0, 0) != 0) {
                    Lua::lua_pop(L, 1);
                }
            }
            Lua::lua_pop(L, 3);
        }
        else {
            Lua::lua_pop(L, 1);
            Lua::lua_pushcfunction(L, lua_QueueInteract);
            if (Lua::lua_isfunction(L, -1)) {
                if (Lua::lua_pcall(L, 0, 0, 0) != 0) {
                    Lua::lua_pop(L, 1);
                }
            }
            Lua::lua_pop(L, 1);
        }
        return 0;
    }

    int lua_openmisclib(lua_State* L) {
        Lua::luaL_Reg funcs[] = {
            { "FlashWindow", lua_FlashWindow },
            { "IsWindowFocused",lua_IsWindowFocused },
            { "FocusWindow", lua_FocusWindow },
            { "CopyToClipboard", lua_CopyToClipboard },
            { "QueueInteract", lua_QueueInteract }
        };

        for (const auto& [name, func] : funcs) {
            Lua::lua_pushcfunction(L, func);
            Lua::lua_setglobal(L, name);
        }
        Hooks::FrameScript::registerOnUpdate(ProcessQueuedInteraction);
        return 0;
    }
    int CVarHandler_interactionAngle(CVar* cvar, const char*, const char* value, void*) { return cvar->Sync(value, &g_iAngle, 15, 160, "%d"); }
    int CVarHandler_interactionMode(CVar* cvar, const char*, const char* value, void*) { return cvar->Sync(value, &g_iMode, 0, 1, "%d"); }

    bool TerrainClick(float x, float y, float z) {
        TerrainClickEvent tc = {
	        .m_guid = 0,
	        .m_pos ={.X=x, .Y=y, .Z=z},
	        .m_button = 1
        };
        CGGameUI::HandleTerrainClickFn(&tc);
        return true;
    }

    int __cdecl SecureCmdOptionParseHk(lua_State* L) {
        int result = CGGameUI::SecureCmdOptionParseFn(L);

        if (Lua::lua_gettop(L) < 3 || !Lua::lua_isstring(L, 2) || !Lua::lua_isstring(L, 3)) return result;

        std::string_view parsed_target_view = Lua::lua_tostring(L, 3);
        bool is_cursor = iequals(parsed_target_view, "cursor");
        bool is_playerlocation = iequals(parsed_target_view, "playerlocation");

        if (!is_cursor && !is_playerlocation) return result;

        if (is_cursor) g_cursorKeywordActive = true;
        else if (is_playerlocation) g_playerLocationKeywordActive = true;

        std::string parsed_result = Lua::lua_tostring(L, 2);
        std::string orig_string = Lua::lua_tostring(L, 1);

        Lua::lua_pop(L, 3);
        Lua::lua_pushstring(L, orig_string.c_str());
        Lua::lua_pushstring(L, parsed_result.c_str());
        Lua::lua_pushnil(L);

        return result;
    }

    int __fastcall OnLayerTrackTerrainHk(CGWorldFrame* pThis, void* edx, int* a1) {
        CGPlayer_C* player = ObjectMgr::Get<CGPlayer_C>(ObjectMgr::GetPlayerGuid(), TYPEMASK_PLAYER);
        if (!player) return pThis->OnLayerTrackTerrain(a1);

        if (g_playerLocationKeywordActive) {
            C3Vector playerPos;
            player->GetPosition(playerPos);

            TerrainClick(playerPos.X, playerPos.Y, playerPos.Z);
            g_playerLocationKeywordActive = false;

            return pThis->OnLayerTrackTerrain(a1);
        }
        if (g_cursorKeywordActive) {
            auto* coords = reinterpret_cast<float*>(a1);
            C3Vector cursorPos = {
                .X = coords[2],
                .Y = coords[3],
                .Z = coords[4]
            };
            TerrainClick(cursorPos.X, cursorPos.Y, cursorPos.Z);
            g_cursorKeywordActive = false;

            return pThis->OnLayerTrackTerrain(a1);
        }
        return pThis->OnLayerTrackTerrain(a1);
    }

    auto SpellCastResetFn = reinterpret_cast<DummyCallback_t>(0x007FEE99);
    constexpr uintptr_t SpellCastReset_jmpback = 0x007FEE9E;

    void __cdecl ResetKeywordFlags() {
        g_cursorKeywordActive = false;
        g_playerLocationKeywordActive = false;
    }

    void __declspec(naked) SpellCastResetHk() {
        __asm {
            call ResetKeywordFlags;
            call CGGameUI::CursorReleaseSpellTargetingFn;
            jmp SpellCastReset_jmpback;
        }
    }

    void OnEnterWorld() {
        Lua::RegisterSlashCommand("INTERACTCMD", "/interact", InteractFunction_C);
        Lua::RegisterLuaBinding("AWESOME_KEYBIND", "INTERACTIONKEYBIND", "Interaction Button", "AWESOME_WOTLK_KEYBINDS", "Awesome Wotlk Keybinds", "QueueInteract()");
    }
}

void Misc::initialize() {
    Hooks::FrameXML::registerLuaLib(lua_openmisclib);
    Hooks::FrameXML::registerCVar(&s_cvar_interactionAngle, "interactionAngle", nullptr, "60", CVarHandler_interactionAngle);
    Hooks::FrameXML::registerCVar(&s_cvar_interactionMode, "interactionMode", nullptr, "1", CVarHandler_interactionMode);

    Hooks::Detour(&CGGameUI::SecureCmdOptionParseFn, SecureCmdOptionParseHk);
    Hooks::Detour(&CGWorldFrame::OnLayerTrackTerrainFn, OnLayerTrackTerrainHk);
    Hooks::Detour(&SpellCastResetFn, SpellCastResetHk);

    Hooks::FrameScript::registerOnEnter(OnEnterWorld);
}