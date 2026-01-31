#include "Hooks.h"
#include <Windows.h>
#include <Detours/detours.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace VoiceChat {
    void shutdown();  // Forward-Declaration
}

static std::unordered_map<void*, float> g_models_original_alphas;
static std::unordered_set<void*> g_models_current;
static std::unordered_set<void*> g_models_being_faded;
static std::unordered_map<void*, uint32_t> g_models_grace_timers;

static uint32_t g_models_cleanup_timer = 0;
static float g_actual_distance = 1.0f;

static Console::CVar* s_cvar_cameraIndirectVisibility;
static Console::CVar* s_cvar_cameraIndirectAlpha;

static int CVarHandler_cameraIndirectVisibility(Console::CVar*, const char*, const char* value, LPVOID) {
    if (!std::atoi(value)) {
        for (void* modelPtr : g_models_being_faded) {
            if (g_models_original_alphas.find(modelPtr) != g_models_original_alphas.end()) {
                float* alphaPtr = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(modelPtr) + 0x17C);
                *alphaPtr = g_models_original_alphas[modelPtr];
            }
        }
        g_models_being_faded.clear();
        g_models_original_alphas.clear();
        g_models_current.clear();
    }
    return 1;
}
static int CVarHandler_cameraIndirectAlpha(Console::CVar* cvar, const char*, const char* value, void*)
{
    float alpha = std::atof(value);
    if (alpha < 0.6f || alpha > 1.0f)
        return 0;
    return 1;
}

static constexpr float MAX_TRACE_DISTANCE = 1000.0f;
static constexpr uint32_t TERRAIN_HIT_FLAGS = 0x100171;
static bool g_cursorKeywordActive = false;
static bool g_playerLocationKeywordActive = false;

static float clampf(float val, float minVal, float maxVal) { return (val < minVal) ? minVal : (val > maxVal) ? maxVal : val; }
static float dot(const C3Vector& a, const C3Vector& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }


typedef int(__thiscall* ProcessAoETargeting)(void* thisPtr, int* a1);
static ProcessAoETargeting ProcessAoETargeting_orig = (ProcessAoETargeting)0x004F66C0;

typedef int(__cdecl* GetSpellRange)(int, int, float*, float*, int);
static GetSpellRange GetSpellRange_orig = (GetSpellRange)0x00802C30;

typedef int(__cdecl* SecureCmdOptionParse_t)(lua_State* L);
static SecureCmdOptionParse_t SecureCmdOptionParse_orig = (SecureCmdOptionParse_t)0x00564AE0;

typedef void(__cdecl* HandleTerrainClick_t)(TerrainClickEvent*);
static HandleTerrainClick_t HandleTerrainClick_orig = (HandleTerrainClick_t)0x00527830;

typedef uint8_t(__cdecl* TraceLine_t)(C3Vector* start, C3Vector* end, C3Vector* hitPoint, float* distance, uint32_t flag, uint32_t optional);
static TraceLine_t TraceLine_orig = (TraceLine_t)0x007A3B70;

typedef char(__cdecl* CGWorldFrame_Intersect_t)(C3Vector* start, C3Vector* end, C3Vector* hitPoint, float* distance, uint32_t flag, uint32_t optional);
static CGWorldFrame_Intersect_t CGWorldFrame_Intersect_orig = (CGWorldFrame_Intersect_t)0x0077F310;

typedef void(__cdecl* ReleaseAoETargetingCircle_t)();
static ReleaseAoETargetingCircle_t ReleaseAoETargetingCircle_orig = (ReleaseAoETargetingCircle_t)0x007FCC30;

struct CVarArgs {
    Console::CVar** dst;
    const char* name;
    const char* desc;
    Console::CVarFlags flags;
    const char* initialValue;
    Console::CVar::Handler_t func;
};

static std::vector<CVarArgs> s_customCVars;
void Hooks::FrameXML::registerCVar(Console::CVar** dst, const char* str, const char* desc, Console::CVarFlags flags, const char* initialValue, Console::CVar::Handler_t func)
{
    s_customCVars.push_back({ dst, str, desc, flags, initialValue, func });
}

static void(*CVars_Initialize_orig)() = (decltype(CVars_Initialize_orig))0x0051D9B0;
static void CVars_Initialize_hk()
{
    CVars_Initialize_orig();
    for (const auto& [dst, name, desc, flags, initialValue, func] : s_customCVars) {
        Console::CVar* cvar = Console::RegisterCVar(name, desc, flags, initialValue, func, 0, 0, 0, 0);
        if (dst) *dst = cvar;
    }
}


static std::vector<const char*> s_customEvents;
void Hooks::FrameXML::registerEvent(const char* str) { s_customEvents.push_back(str); }

static void (*FrameScript_FillEvents_orig)(const char** list, size_t count) = (decltype(FrameScript_FillEvents_orig))0x0081B5F0;
static void FrameScript_FillEvents_hk(const char** list, size_t count)
{
    std::vector<const char*> events;
    events.reserve(count + s_customEvents.size());
    events.insert(events.end(), &list[0], &list[count]);
    events.insert(events.end(), s_customEvents.begin(), s_customEvents.end());
    FrameScript_FillEvents_orig(events.data(), events.size());
}


static std::vector<lua_CFunction> s_customLuaLibs;
void Hooks::FrameXML::registerLuaLib(lua_CFunction func) { s_customLuaLibs.push_back(func); }

static void Lua_OpenFrameXMlApi_bulk()
{
    lua_State* L = GetLuaState();
    for (auto& func : s_customLuaLibs)
        func(L);
}

static void(*Lua_OpenFrameXMLApi_orig)() = (decltype(Lua_OpenFrameXMLApi_orig))0x00530F85;
static void __declspec(naked) Lua_OpenFrameXMLApi_hk()
{
    __asm {
        pushad;
        pushfd;
        call Lua_OpenFrameXMlApi_bulk;
        popfd;
        popad;
        ret;
    }
}

struct CustomTokenDetails {
    CustomTokenDetails() { memset(this, NULL, sizeof(*this)); }
    CustomTokenDetails(Hooks::FrameScript::TokenGuidGetter* getGuid, Hooks::FrameScript::TokenIdGetter* getId)
        : hasN(false), getGuid(getGuid), getId(getId)
    {
    }
    CustomTokenDetails(Hooks::FrameScript::TokenNGuidGetter* getGuid, Hooks::FrameScript::TokenIdNGetter* getId)
        : hasN(true), getGuidN(getGuid), getIdN(getId)
    {
    }

    bool hasN;
    union {
        Hooks::FrameScript::TokenGuidGetter* getGuid;
        Hooks::FrameScript::TokenNGuidGetter* getGuidN;
    };
    union {
        Hooks::FrameScript::TokenIdGetter* getId;
        Hooks::FrameScript::TokenIdNGetter* getIdN;
    };
};
static std::unordered_map<std::string, CustomTokenDetails> s_customTokens;
void Hooks::FrameScript::registerToken(const char* token, TokenGuidGetter* getGuid, TokenIdGetter* getId) { s_customTokens[token] = { getGuid, getId }; }
void Hooks::FrameScript::registerToken(const char* token, TokenNGuidGetter* getGuid, TokenIdNGetter* getId) { s_customTokens[token] = { getGuid, getId }; }

static DWORD_PTR GetGuidByKeyword_jmpbackaddr = 0;
static void GetGuidByKeyword_bulk(const char** stackStr, guid_t* guid)
{
    for (auto& [token, conv] : s_customTokens) {
        if (strncmp(*stackStr, token.data(), token.size()) == 0) {
            *stackStr += token.size();
            if (conv.hasN) {
                int n = gc_atoi(stackStr);
                *guid = n > 0 ? conv.getGuidN(n - 1) : 0;
            }
            else {
                *guid = conv.getGuid();
            }
            GetGuidByKeyword_jmpbackaddr = 0x0060AD57;
            return;
        }
    }
    GetGuidByKeyword_jmpbackaddr = 0x0060AD44;
}

static void(*GetGuidByKeyword_orig)() = (decltype(GetGuidByKeyword_orig))0x0060AFAA;
static void __declspec(naked) GetGuidByKeyword_hk()
{
    __asm {
        pushad;
        pushfd;
        push[ebp + 0xC];
        lea eax, [ebp + 0x8];
        push eax;
        call GetGuidByKeyword_bulk;
        add esp, 8;
        popfd;
        popad;

        push GetGuidByKeyword_jmpbackaddr;
        ret;
    }
}

static char** (*GetKeywordsByGuid_orig)(guid_t* guid, size_t* size) = (decltype(GetKeywordsByGuid_orig))0x0060BB70;
static char** GetKeywordsByGuid_hk(guid_t* guid, size_t* size)
{
    char** buf = GetKeywordsByGuid_orig(guid, size);
    if (!buf) return buf;
    for (auto& [token, conv] : s_customTokens) {
        if (*size >= 8) break;
        if (conv.hasN) {
            int id = conv.getIdN(*guid);
            if (id >= 0)
                snprintf(buf[(*size)++], 32, "%s%d", token.c_str(), id + 1);
        }
        else {
            if (conv.getId(*guid))
                snprintf(buf[(*size)++], 32, "%s", token.c_str());
        }
    }
    return buf;
}

static std::vector<Hooks::DummyCallback_t> s_customOnUpdate;
void Hooks::FrameScript::registerOnUpdate(DummyCallback_t func) { s_customOnUpdate.push_back(func); }

static std::vector<Hooks::DummyCallback_t> s_customOnEnter;
void Hooks::FrameScript::registerOnEnter(DummyCallback_t func) { s_customOnEnter.push_back(func); }

static std::vector<Hooks::DummyCallback_t> s_customOnLeave;
void Hooks::FrameScript::registerOnLeave(DummyCallback_t func) { s_customOnLeave.push_back(func); }

static int(*FrameScript_FireOnUpdate_orig)(int a1, int a2, int a3, int a4) = (decltype(FrameScript_FireOnUpdate_orig))0x00495810;
static int FrameScript_FireOnUpdate_hk(int a1, int a2, int a3, int a4)
{
    for (auto func : s_customOnUpdate)
        func();
    return FrameScript_FireOnUpdate_orig(a1, a2, a3, a4);
}

static void(__fastcall* CGGameUI__EnterWorld)() = (decltype(CGGameUI__EnterWorld))0x00528010;
static void __fastcall OnEnterWorld()
{
    for (auto func : s_customOnEnter)
        func();
    return CGGameUI__EnterWorld();
}

static void(__fastcall* CGGameUI__LeaveWorld)() = (decltype(CGGameUI__LeaveWorld))0x00528C30;
static void __fastcall OnLeaveWorld()
{
    for (auto func : s_customOnLeave)
        func();
    return CGGameUI__LeaveWorld();
}

static std::vector<Hooks::DummyCallback_t> s_glueXmlPostLoad;
void Hooks::GlueXML::registerPostLoad(DummyCallback_t func) { s_glueXmlPostLoad.push_back(func); }

static void LoadGlueXML_bulk() { for (auto func : s_glueXmlPostLoad) func(); }

static void (*LoadGlueXML_orig)() = (decltype(LoadGlueXML_orig))0x004DA9AC;
static void __declspec(naked) LoadGlueXML_hk()
{
    __asm {
        pop ebx;
        mov esp, ebp;
        pop ebp;

        pushad;
        pushfd;
        call LoadGlueXML_bulk;
        popfd;
        popad;
        ret;
    }
}


static std::vector<Hooks::DummyCallback_t> s_glueXmlCharEnum;
void Hooks::GlueXML::registerCharEnum(DummyCallback_t func) { s_glueXmlCharEnum.push_back(func); }

static void LoadCharacters_bulk()
{
    for (auto func : s_glueXmlCharEnum)
        func();
}

static void (*LoadCharacters_orig)() = (decltype(LoadCharacters_orig))0x004E47E5;
static void __declspec(naked) LoadCharacters_hk()
{
    __asm {
        add esp, 8;
        pop esi;

        pushad;
        pushfd;
        call LoadCharacters_bulk;
        popfd;
        popad;

        ret;
    }
}

static bool TerrainClick(float x, float y, float z) {
    TerrainClickEvent tc = {};
    tc.GUID = 0;
    tc.x = x;
    tc.y = y;
    tc.z = z;
    tc.button = 1;

    HandleTerrainClick_orig(&tc);
    return true;
}

static bool TraceLine(const C3Vector& start, const C3Vector& end, uint32_t hitFlags,
    C3Vector& intersectionPoint, float& completedBeforeIntersection) {
    completedBeforeIntersection = 1.0f;
    intersectionPoint = { 0.0f, 0.0f, 0.0f };

    uint8_t result = TraceLine_orig(
        const_cast<C3Vector*>(&start),
        const_cast<C3Vector*>(&end),
        &intersectionPoint,
        &completedBeforeIntersection,
        hitFlags,
        0
    );

    if (result != 0 && result != 1)
        return false;

    completedBeforeIntersection *= 100.0f;
    return static_cast<bool>(result);
}

static bool GetCursorWorldPosition(VecXYZ& worldPos) {
    Camera* camera = GetActiveCamera();
    if (!camera)
        return false;

    DWORD basePtr = *(DWORD*)UIBase;
    if (!basePtr)
        return false;

    float nx = *reinterpret_cast<float*>(basePtr + 4644) * 2.0f - 1.0f; // x perc
    float ny = *reinterpret_cast<float*>(basePtr + 4648) * 2.0f - 1.0f; // y perc

    float tanHalfFov = tanf(camera->fovInRadians * 0.3f);
    VecXYZ localRay = {
        nx * camera->aspect * tanHalfFov,
        ny * tanHalfFov,
        1.0f
    };

    const float* cameraMatrix = reinterpret_cast<const float*>(reinterpret_cast<uintptr_t>(camera) + 0x14);

    VecXYZ dir;
    dir.x = (-cameraMatrix[3]) * localRay.x + cameraMatrix[6] * localRay.y + cameraMatrix[0] * localRay.z;
    dir.y = (-cameraMatrix[4]) * localRay.x + cameraMatrix[7] * localRay.y + cameraMatrix[1] * localRay.z;
    dir.z = (-cameraMatrix[5]) * localRay.x + cameraMatrix[8] * localRay.y + cameraMatrix[2] * localRay.z;

    VecXYZ farPoint = {
        camera->pos.x + dir.x * MAX_TRACE_DISTANCE,
        camera->pos.y + dir.y * MAX_TRACE_DISTANCE,
        camera->pos.z + dir.z * MAX_TRACE_DISTANCE
    };

    C3Vector start = { camera->pos.x, camera->pos.y, camera->pos.z };
    C3Vector end = { farPoint.x, farPoint.y, farPoint.z };
    C3Vector hitPoint;
    float distance;

    bool hit = TraceLine(start, end, TERRAIN_HIT_FLAGS, hitPoint, distance);
    if (hit) {
        worldPos.x = hitPoint.X;
        worldPos.y = hitPoint.Y;
        worldPos.z = hitPoint.Z;
        return true;
    }

    return false;
}

/*inline int __fastcall Spell_C_CancelPlayerSpells() {
    return ((decltype(&Spell_C_CancelPlayerSpells))0x00809AC0)();
}*/

static bool isSpellReadied() {
    unsigned int spellTargetingFlag = *(unsigned int*)0x00D3F4E4;
    unsigned int spellTargetingState = *(unsigned int*)0x00D3F4E0;
    return spellTargetingFlag != 0 && (spellTargetingState & 0x40) != 0;
}

static int __cdecl SecureCmdOptionParse_hk(lua_State* L) {
    int result = SecureCmdOptionParse_orig(L);

    if (lua_gettop(L) < 3 || !lua_isstring(L, 2) || !lua_isstring(L, 3))
        return result;

    std::string_view parsed_target_view = lua_tostringnew(L, 3);
    bool is_cursor = std::equal(parsed_target_view.begin(), parsed_target_view.end(),
        "cursor", "cursor" + 6,
        [](char a, char b) { return std::tolower(a) == b; });
    bool is_playerlocation = std::equal(parsed_target_view.begin(), parsed_target_view.end(),
        "playerlocation", "playerlocation" + 14,
        [](char a, char b) { return std::tolower(a) == b; });

    if (!is_cursor && !is_playerlocation)
        return result;

    if (is_cursor) {
        g_cursorKeywordActive = true;
    }
    else if (is_playerlocation) {
        g_playerLocationKeywordActive = true;
    }

    std::string parsed_result = lua_tostringnew(L, 2);
    std::string orig_string = lua_tostringnew(L, 1);

    lua_pop(L, 3);
    lua_pushstring(L, orig_string.c_str());
    lua_pushstring(L, parsed_result.c_str());
    lua_pushnil(L);

    return result;
}

static int __fastcall ProcessAoETargeting_hk(
    void* thisPtr,
    void* /*edx*/,
    int* a1
)
{

    CGUnit_C* player = ObjectMgr::GetCGUnitPlayer();
    if (!player)
        return ProcessAoETargeting_orig(thisPtr, a1);

    if (g_playerLocationKeywordActive)
    {
        C3Vector playerPos;
        player->GetPosition(playerPos);

        TerrainClick(playerPos.X, playerPos.Y, playerPos.Z);
        g_playerLocationKeywordActive = false;

        return ProcessAoETargeting_orig(thisPtr, a1);
    }

    if (g_cursorKeywordActive)
    {
        C3Vector cursorPos{
            *(float*)&a1[2],
            *(float*)&a1[3],
            *(float*)&a1[4]
        };

        TerrainClick(cursorPos.X, cursorPos.Y, cursorPos.Z);
        g_cursorKeywordActive = false;

        return ProcessAoETargeting_orig(thisPtr, a1);
    }

    return ProcessAoETargeting_orig(thisPtr, a1);
}

static char __cdecl CGWorldFrame_Intersect_new(C3Vector* playerPos, C3Vector* cameraPos, C3Vector* hitPoint, float* hitDistance, uint32_t hitFlags, uint32_t buffer)
{
    if (!std::atoi(s_cvar_cameraIndirectVisibility->vStr))
        return CGWorldFrame_Intersect_orig(playerPos, cameraPos, hitPoint, hitDistance, hitFlags + 1, 0);

    void* buf = reinterpret_cast<void*>(buffer);
    std::memset(buf, 0, 2048);

    char result = CGWorldFrame_Intersect_orig(playerPos, cameraPos, hitPoint, hitDistance, hitFlags + 1 + 0x8000000, reinterpret_cast<uintptr_t>(buf));
    if (result) {
        const uint32_t* bufData = reinterpret_cast<const uint32_t*>(buf);
        if ((bufData[0] == 1 || bufData[0] == 2) && bufData[1] > 0) {
            void* modelPtr = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(buf) + 12 + 80);
            if (modelPtr) {
                g_models_current.insert(modelPtr);
                g_models_being_faded.insert(modelPtr);

                float* alphaPtr = reinterpret_cast<float*>(static_cast<char*>(modelPtr) + 0x17C);

                if (g_models_original_alphas.find(modelPtr) == g_models_original_alphas.end())
                    g_models_original_alphas[modelPtr] = *alphaPtr;

                *alphaPtr += (std::atof(s_cvar_cameraIndirectAlpha->vStr) - *alphaPtr) * 0.25f;

                g_models_grace_timers[modelPtr] = 0;

                if (g_actual_distance < 1.0f)
                    *hitDistance = g_actual_distance;
                else
                    result = false;
            }
        }
        else {
            g_models_current.clear();
            g_actual_distance = *hitDistance;
        }
    }
    else if (g_models_being_faded.empty()) {
        return result;
    }
    else {
        g_models_current.clear();
        g_actual_distance = 1.0f;
    }

    uint32_t cleanup_timer = g_models_being_faded.size() + 1;
    for (auto it = g_models_being_faded.begin(); it != g_models_being_faded.end();) {
        void* modelPtr = *it;

        if (g_models_grace_timers[modelPtr] > cleanup_timer) {
            float* alphaPtr = reinterpret_cast<float*>(static_cast<char*>(modelPtr) + 0x17C);
            float originalAlpha = g_models_original_alphas[modelPtr];
            *alphaPtr += (originalAlpha - *alphaPtr) * 0.25f;
            if (std::fabs(*alphaPtr - originalAlpha) < 0.01f) {
                *alphaPtr = originalAlpha;
                g_models_original_alphas.erase(modelPtr);
                it = g_models_being_faded.erase(it);
                continue;
            }
        }
        ++it;
    }

    for (auto& pair : g_models_grace_timers)
        pair.second++;

    return result;
}

static void(*IntersectCall_orig)() = (decltype(IntersectCall_orig))0x006060E6;
static constexpr DWORD_PTR IntersectCall_jmpbackaddr = 0x00606103;

static void __declspec(naked) IntersectCall_hk()
{
    __asm {
        sub esp, 2048
        lea eax, [esp]
        fld1
        push eax
        and ebx, ~1
        push ebx
        fstp dword ptr[ebp + 0x18]
        lea ecx, [ebp + 0x18]
        push ecx
        lea edx, [ebp - 0x48]
        push edx
        lea eax, [ebp - 0x18]
        push eax
        lea ecx, [ebp - 0x24]
        push ecx
        call CGWorldFrame_Intersect_new
        add esp, 2048
        jmp IntersectCall_jmpbackaddr
    }
}

static bool __cdecl collisionFilter(void* modelPtr) {
    return g_models_current.find(modelPtr) == g_models_current.end();
}

static void(*IterateCollisionList_orig)() = (decltype(IterateCollisionList_orig))0x007A279D;
static constexpr DWORD_PTR IterateCollisionList_jmpback = 0x007A27A5;
static constexpr DWORD_PTR IterateCollisionList_skipaddr = 0x007A2943;

static __declspec(naked) void IterateCollisionList_hk()
{
    __asm {
        mov esi, [edx + 4]       // esi = object entry
        pushfd
        push eax
        push ecx
        push edx
        mov eax, [ebp + 0Ch]
        and eax, 0x8000000
        jz run_collision_processing
        mov eax, [esi + 0x34]    // eax = modelPtr
        test eax, eax
        jz skip_collision_processing
        push eax
        call collisionFilter
        add esp, 4
        test al, al
        je skip_collision_processing
        run_collision_processing :
            pop edx
            pop ecx
            pop eax
            popfd
            cmp byte ptr[esi + 25h], bl
            jmp IterateCollisionList_jmpback
        skip_collision_processing :
            pop edx
            pop ecx
            pop eax
            popfd
            jmp IterateCollisionList_skipaddr
    }
}

static void(*IterateWorldObjCollisionList_orig)() = (decltype(IterateWorldObjCollisionList_orig))0x007A2A1C;
static constexpr DWORD_PTR IterateWorldObjCollisionList_jmpback = 0x007A2A23;
static constexpr DWORD_PTR IterateWorldObjCollisionList_skipaddr = 0x007A2A8A;

static __declspec(naked) void IterateWorldObjCollisionList_hk()
{
    __asm {
        mov edx, [ebp + 0Ch]
        test edx, 0x8000000
        jz run_collision_processing
        push eax    // eax = modelPtr
        call collisionFilter
        add esp, 4
        test al, al
        je skip_collision_processing
        run_collision_processing :
            mov eax, [ebp - 68h]
            cmp dword ptr[eax + 2D8h], 0
            jmp IterateWorldObjCollisionList_jmpback
        skip_collision_processing :
            jmp IterateWorldObjCollisionList_skipaddr
    }
}


static void(*SpellCastReset_orig)() = (decltype(SpellCastReset_orig))0x007FEE99;
static constexpr DWORD_PTR SpellCastReset_jmpback = 0x007FEE9E;

static void __cdecl ResetKeywordFlags() {
    g_cursorKeywordActive = false;
    g_playerLocationKeywordActive = false;
}

static void __declspec(naked) SpellCastReset_hk() {
    __asm {
        call ResetKeywordFlags
        call ReleaseAoETargetingCircle_orig
        jmp SpellCastReset_jmpback
    }
}

using tCGameDestroy = void(__thiscall*)(void* self);
static tCGameDestroy CGame_Destroy_orig =
    (decltype(CGame_Destroy_orig))0x00406B70;

static void __declspec(naked) CGame_Destroy_hk()
{
    __asm {
        pushad
        pushfd
    }

    VoiceChat::shutdown();

    __asm {
        popfd
        popad
        mov eax, CGame_Destroy_orig
        jmp eax
    }
}

void Hooks::initialize()
{
    Hooks::FrameXML::registerCVar(&s_cvar_cameraIndirectAlpha, "cameraIndirectAlpha", NULL, (Console::CVarFlags)1, "0.6", CVarHandler_cameraIndirectAlpha);
    Hooks::FrameXML::registerCVar(&s_cvar_cameraIndirectVisibility, "cameraIndirectVisibility", NULL, (Console::CVarFlags)1, "0", CVarHandler_cameraIndirectVisibility);
    DetourAttach(&(LPVOID&)CVars_Initialize_orig, CVars_Initialize_hk);
    DetourAttach(&(LPVOID&)FrameScript_FireOnUpdate_orig, FrameScript_FireOnUpdate_hk);
    DetourAttach(&(LPVOID&)CGGameUI__EnterWorld, OnEnterWorld);
    DetourAttach(&(LPVOID&)CGGameUI__LeaveWorld, OnLeaveWorld);
    DetourAttach(&(LPVOID&)FrameScript_FillEvents_orig, FrameScript_FillEvents_hk);
    DetourAttach(&(LPVOID&)Lua_OpenFrameXMLApi_orig, Lua_OpenFrameXMLApi_hk);
    DetourAttach(&(LPVOID&)GetGuidByKeyword_orig, GetGuidByKeyword_hk);
    DetourAttach(&(LPVOID&)GetKeywordsByGuid_orig, GetKeywordsByGuid_hk);
    DetourAttach(&(LPVOID&)LoadGlueXML_orig, LoadGlueXML_hk);
    DetourAttach(&(LPVOID&)LoadCharacters_orig, LoadCharacters_hk);
    DetourAttach(&(LPVOID&)SecureCmdOptionParse_orig, SecureCmdOptionParse_hk);
    DetourAttach(&(LPVOID&)ProcessAoETargeting_orig, ProcessAoETargeting_hk);
    DetourAttach(&(LPVOID&)IterateCollisionList_orig, IterateCollisionList_hk);
    DetourAttach(&(LPVOID&)IterateWorldObjCollisionList_orig, IterateWorldObjCollisionList_hk);
    DetourAttach(&(LPVOID&)IntersectCall_orig, IntersectCall_hk);
    DetourAttach(&(LPVOID&)SpellCastReset_orig, SpellCastReset_hk);
    DetourAttach(&(LPVOID&)CGame_Destroy_orig, CGame_Destroy_hk);
}
