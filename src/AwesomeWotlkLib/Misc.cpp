#include "Misc.h"
#include "GameClient.h"
#include "Hooks.h"
#include "Utils.h"
#include <Windows.h>
#include <Detours/detours.h>

#undef min
#undef max

static constexpr double M_PI = 3.14159265358979323846;
static Console::CVar* s_cvar_interactionMode;
static Console::CVar* s_cvar_interactionAngle;
static Console::CVar* s_cvar_cameraFov = NULL;
static Console::CVar* s_cvar_showPlayer;

static uint32_t gCvar_HidePlayer = 1;

static int lua_FlashWindow(lua_State* L)
{
    HWND hwnd = GetGameWindow();
    if (hwnd) FlashWindow(hwnd, FALSE);
    return 0;
}

static int lua_IsWindowFocused(lua_State* L)
{
    HWND hwnd = GetGameWindow();
    if (!hwnd || GetForegroundWindow() != hwnd)
        return 0;
    lua_pushnumber(L, 1.f);
    return 1;
}

static int lua_FocusWindow(lua_State* L)
{
    HWND hwnd = GetGameWindow();
    if (hwnd) SetForegroundWindow(hwnd);
    return 0;
}

static int lua_CopyToClipboard(lua_State* L)
{
    const char* str = luaL_checkstring(L, 1);
    if (str && str[0]) CopyToClipboardU8(str, NULL);
    return 0;
}

static guid_t s_requestedInteraction = 0;
static void ProcessQueuedInteraction()
{
    if (!s_requestedInteraction)
        return;

    CGObject_C* object = ObjectMgr::GetObjectPtr(s_requestedInteraction, TYPEMASK_GAMEOBJECT | TYPEMASK_UNIT);
    if (object) {
        object->OnRightClick(); // safe internal call, no Lua taint
    }

    s_requestedInteraction = 0;
}

static bool IsGoodObject(uint8_t gameObjectType)
{
    switch (gameObjectType)
    {
        case GAMEOBJECT_TYPE_DOOR:
        case GAMEOBJECT_TYPE_BUTTON:
        case GAMEOBJECT_TYPE_QUESTGIVER:
        case GAMEOBJECT_TYPE_CHEST:
        case GAMEOBJECT_TYPE_BINDER:
        case GAMEOBJECT_TYPE_CHAIR:
        case GAMEOBJECT_TYPE_SPELL_FOCUS:
        case GAMEOBJECT_TYPE_GOOBER:
        case GAMEOBJECT_TYPE_FISHINGNODE:
        case GAMEOBJECT_TYPE_SUMMONING_RITUAL:
        case GAMEOBJECT_TYPE_MAILBOX:
        case GAMEOBJECT_TYPE_MEETINGSTONE:
        case GAMEOBJECT_TYPE_FLAGSTAND:
        case GAMEOBJECT_TYPE_FLAGDROP:
        case GAMEOBJECT_TYPE_BARBER_CHAIR:
        case GAMEOBJECT_TYPE_GUILD_BANK:
        case GAMEOBJECT_TYPE_TRAPDOOR:
            return true;  
        default:
            return false; 
    }
}

static int lua_QueueInteract(lua_State* L)
{
    std::string modifier = "";
    bool hasModifier = !lua_isnoneornil(L, 1);

    if (hasModifier) {
        const char* raw = lua_tostringnew(L, 1);
        if (!raw) {
            return 0;
        }

        std::string modStr = raw;

        for (char c : modStr) {
            if (!std::isalnum(static_cast<unsigned char>(c))) {
                return 0;
            }
        }

        modifier = modStr;
    }

    if (!IsInWorld())
        return 0;

    guid_t candidate = 0;
    float bestDistance = 3000.0f;

    CGUnit_C* player = ObjectMgr::GetCGUnitPlayer();
    if (!player) {
        return 0;
    }

    if (!player) return 0;


    auto isValidObject = [&](CGObject_C* object) -> bool {
        if (object->GetTypeID() == TYPEID_UNIT) {
            uint32_t dynFlags = object->GetValue<uint32_t>(UNIT_DYNAMIC_FLAGS);
            uint32_t unitFlags = object->GetValue<uint32_t>(UNIT_FIELD_FLAGS);
            uint32_t npcFlags = object->GetValue<uint32_t>(UNIT_NPC_FLAGS);

            bool isLootable = (dynFlags & UNIT_DYNFLAG_LOOTABLE) != 0;
            bool isSkinnable = (unitFlags & UNIT_FLAG_SKINNABLE) != 0;
            bool canAssist = player->CanAssist((CGUnit_C*)object, true);

            if (!isLootable && !isSkinnable && (!canAssist || npcFlags == 0)) {
                return false;
            }
            return true;
        }else if (object->GetTypeID() == TYPEID_GAMEOBJECT) {
            uint32_t bytes = object->GetValue<uint32_t>(GAMEOBJECT_BYTES_1);
            uint8_t gameObjectType = (bytes >> 8) & 0xFF;

            if (!IsGoodObject(gameObjectType)) return false;

            if (!static_cast<CGGameObject_C*>(object)->CanUseNow()) {
                return false;
            }
            return true;
        }
        return false;
    };

    uint16_t angleDegrees = std::atoi(s_cvar_interactionAngle->vStr) / 2;
    bool lookInAngle = std::atoi(s_cvar_interactionMode->vStr) == 1;

    VecXYZ posPlayer;
    player->GetPosition(*reinterpret_cast<C3Vector*>(&posPlayer));

    auto trySetCandidate = [&](guid_t guid) {
        CGObject_C* object = ObjectMgr::GetObjectPtr(guid, TYPEMASK_GAMEOBJECT | TYPEMASK_UNIT);
        if (!object) return;

        float distance = player->distance(object);
        if (distance == 0.f || distance > 20.0f || distance > bestDistance) return;

        if (!isValidObject(object)) return;

        if (lookInAngle) {
            VecXYZ posObject;
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

            float dot = dx * fx + dy * fy;

            float cosAngle = cosf(angleDegrees * (3.14159265f / 180.0f));
            if (dot < cosAngle) return;
        }

        candidate = guid;
        bestDistance = distance;
    };

    if (!hasModifier) {
        ObjectMgr::EnumObjects([&](guid_t guid) {
            if (guid != player->GetGUID()) {
                trySetCandidate(guid);
            }
            return true;
        });
    }else {
        guid_t guid = ObjectMgr::GetGuidByUnitID(modifier.c_str());
        if (guid) {
            trySetCandidate(guid);
        }
    }

    if (candidate != 0)
        s_requestedInteraction = candidate;

    return 0;
}

static int InteractFunction_C(lua_State* L) {
    const char* param = nullptr;
    if (!lua_isnoneornil(L, 1)) {
        param = lua_tostringnew(L, 1);
    }

    lua_getglobal(L, "SecureCmdOptionParse");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        lua_pushcfunction(L, lua_QueueInteract);
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != 0) {
                lua_pop(L, 1);
            }
        }
        return 0;
    }

    if (param)
        lua_pushstring(L, param);
    else
        lua_pushnil(L);

    if (lua_pcall(L, 1, 2, 0) != 0) {
        lua_pop(L, 1);
        lua_pushcfunction(L, lua_QueueInteract);
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != 0) {
                lua_pop(L, 1);
            }
        }
        return 0;
    }

    if (!lua_isnil(L, -1)) {
        lua_pushcfunction(L, lua_QueueInteract);
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, -2);
            if (lua_pcall(L, 1, 0, 0) != 0) {
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 3);
    }
    else {
        lua_pop(L, 1);
        lua_pushcfunction(L, lua_QueueInteract);
        if (lua_isfunction(L, -1)) {
            if (lua_pcall(L, 0, 0, 0) != 0) {
                lua_pop(L, 1);
            }
        }
        lua_pop(L, 1);
    }

    return 0;
}

static int RegisterInteractCommand(lua_State* L) {
    lua_pushcfunction(L, InteractFunction_C);    

    lua_getglobal(L, "SlashCmdList");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 2);
        return 0;
    }

    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "INTERACTCMD");

    lua_pop(L, 2);

    lua_pushstring(L, "/interact");
    lua_setglobal(L, "SLASH_INTERACTCMD1");
    return 0;
}

static int lua_openmisclib(lua_State* L)
{
    luaL_Reg funcs[] = {
        { "FlashWindow", lua_FlashWindow },
        { "IsWindowFocused", lua_IsWindowFocused },
        { "FocusWindow", lua_FocusWindow },
        { "CopyToClipboard", lua_CopyToClipboard },
        { "QueueInteract", lua_QueueInteract }
    };

    for (const auto& [name, func] : funcs) {
        lua_pushcfunction(L, func);
        lua_setglobal(L, name);
    }

    Hooks::FrameScript::registerOnUpdate(ProcessQueuedInteraction);

    return 0;
}

static double parseFov(const char* v) { return  M_PI / 200.f * double(std::max(std::min(gc_atoi(&v), 200), 1)); }
static int CVarHandler_interactionAngle(Console::CVar*, const char*, const char* value, LPVOID) { return 1; }
static int CVarHandler_interactionMode(Console::CVar*, const char*, const char* value, LPVOID) { return 1; }
static int CVarHandler_cameraFov(Console::CVar* cvar, const char* prevV, const char* newV, void* udata)
{
    if (Camera* camera = GetActiveCamera()) camera->fovInRadians = parseFov(newV);
    return 1;
}

static void(__fastcall* Camera_Initialize_orig)(Camera* self, void* edx, float a2, float a3, float fov) = (decltype(Camera_Initialize_orig))0x00607C20;
static void __fastcall Camera_Initialize_hk(Camera* self, void* edx, float a2, float a3, float fov)
{
    fov = parseFov(s_cvar_cameraFov->vStr);
    Camera_Initialize_orig(self, edx, a2, a3, fov);
}

static void updateShowPlayer()
{
    CGUnit_C* player = ObjectMgr::GetCGUnitPlayer();
    if (!player)
        return;

    float* alphaPtr = reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(player) + 0x98);
    *alphaPtr = gCvar_HidePlayer == 0 ? 0.0f : 1.0f;
}
static int CVarHandler_showPlayer(Console::CVar* cvar, const char*, const char* value, void*)
{
    int v = std::atoi(value);
    
    if (v != 0 && v != 1)
        v = 1;

    gCvar_HidePlayer = v; // store as global flag
    updateShowPlayer();
    return 1;
}

static bool triggered = false;
static void OnEnterWorld()
{
    if (!triggered) {
        RegisterInteractCommand(GetLuaState());
        RegisterLuaBinding("INTERACTIONKEYBIND", "Interaction Button", "AWESOME_WOTLK_KEYBINDS", "Awesome Wotlk Keybinds", "QueueInteract()");
        updateShowPlayer();
        triggered = true;
    }
}

static void OnLeaveWorld()
{
    updateShowPlayer();
    triggered = false;
}

void Misc::initialize()
{
    Hooks::FrameXML::registerCVar(&s_cvar_showPlayer, "showPlayer", NULL, (Console::CVarFlags)1, "1", CVarHandler_showPlayer);
    Hooks::FrameXML::registerCVar(&s_cvar_cameraFov, "cameraFov", NULL, (Console::CVarFlags)1, "100", CVarHandler_cameraFov);
    Hooks::FrameXML::registerCVar(&s_cvar_interactionAngle, "interactionAngle", NULL, (Console::CVarFlags)1, "60", CVarHandler_interactionAngle);
    Hooks::FrameXML::registerCVar(&s_cvar_interactionMode, "interactionMode", NULL, (Console::CVarFlags)1, "1", CVarHandler_interactionMode);
    Hooks::FrameXML::registerLuaLib(lua_openmisclib);

    DetourAttach(&(LPVOID&)Camera_Initialize_orig, Camera_Initialize_hk);
    Hooks::FrameScript::registerOnEnter(OnEnterWorld);
    Hooks::FrameScript::registerOnLeave(OnLeaveWorld);
}