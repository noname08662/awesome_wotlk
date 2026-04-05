// ============================================================================
// VoiceChat TTS – Windows SAPI-backed speech + Lua bindings
//
// Provides two Lua globals:
//   1) C_VoiceChat
//        - GetTtsVoices()                -> { {voiceID=..., name="..."}, ... }
//        - GetRemoteTtsVoices()          -> same as GetTtsVoices()
//        - SpeakText(voiceID, text[, destination, rate, volume])
//            destination (number):
//              1 -> speak immediately (no special handling; SAPI queues FIFO)
//              4 -> accepted but not specially handled (also just Speak async)
//        - StopSpeakingText()            -> stops all queued/playing utterances
//
//   2) C_TTSSettings
//        - GetSpeechRate()               -> int [-10..10]
//        - GetSpeechVolume()             -> int [0..100]
//        - GetSpeechVoiceID()            -> int (voice index)
//        - GetVoiceOptionName()          -> string (current voice name)
//        - SetDefaultSettings()          -> voice=1* (if exists), rate=0, vol=100
//        - SetSpeechRate(newVal)
//        - SetSpeechVolume(newVal)
//        - SetVoiceOption(voiceID)
//        - SetVoiceOptionByName(voiceName)
//        - RefreshVoices()               -> fires VOICE_CHAT_TTS_VOICES_UPDATE if list changed
//
// Events:
//   VOICE_CHAT_TTS_PLAYBACK_STARTED (numConsumers:number, utteranceID:number, durationMS:number, destination:number)
//       - Fired when SAPI starts the utterance. durationMS is always 0.
//   VOICE_CHAT_TTS_PLAYBACK_FINISHED (numConsumers:number, utteranceID:number, destination:number)
//       - Fired when SAPI finishes the utterance.
//   VOICE_CHAT_TTS_PLAYBACK_FAILED (status:string, utteranceID:number, destination:number)
//       - Fired if Speak() or setup fails (e.g. no voice/device).
//   VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE (status:string, utteranceID:number)
//       - unused.
//   VOICE_CHAT_TTS_VOICES_UPDATE ()
//       - Fired when the enumerated voice list changes.
//
// Notes:
// * Uses CVars: ttsVoice, ttsSpeed, ttsVolume (with clamping callbacks).
// * SAPI speech is async; stream numbers are mapped to utterance metadata.
// * durationMS in START event is always 0 (Blizzard-like behavior).
// * A single global ISpVoice instance is used.
// ============================================================================

#include "VoiceChat.h"
#include "GameClient.h"
#include "Lua.h"
#include "Utils.h"
#include "Hooks.h"
#include <sapi.h>
#include <sphelper.h>
#include <codecvt>
#include <locale>
#include <vector>
#include <string>
#include <Windows.h>
#include <limits>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <algorithm>

// ============================================================================
// Event Names (FrameScript::FireEvent payloads)
// ============================================================================
#define VOICE_CHAT_TTS_PLAYBACK_FAILED   "VOICE_CHAT_TTS_PLAYBACK_FAILED"   // status, utteranceID, destination
#define VOICE_CHAT_TTS_PLAYBACK_FINISHED "VOICE_CHAT_TTS_PLAYBACK_FINISHED" // numConsumers, utteranceID, destination
#define VOICE_CHAT_TTS_PLAYBACK_STARTED  "VOICE_CHAT_TTS_PLAYBACK_STARTED"  // numConsumers, utteranceID, durationMS, destination
#define VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE "VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE" // status, utteranceID (not used here)
#define VOICE_CHAT_TTS_VOICES_UPDATE     "VOICE_CHAT_TTS_VOICES_UPDATE"

// ============================================================================
// Forward Declarations (to keep helpers independent of definition order)
// ============================================================================
struct VoiceTtsVoiceType;
static std::vector<VoiceTtsVoiceType> VoiceChat_GetTtsVoices();
static void VoiceChat_InitVoice();

// ============================================================================
// Destination constants (kept for API compatibility; no special handling)
// ============================================================================
enum : int {
    DEST_LOCAL_PLAYBACK        = 1,
    DEST_QUEUED_LOCAL_PLAYBACK = 4,
};

static int ClampDestination(int d) {
    return (d == DEST_QUEUED_LOCAL_PLAYBACK) ? DEST_QUEUED_LOCAL_PLAYBACK : DEST_LOCAL_PLAYBACK;
}

// ============================================================================
// CVar Helpers
// ============================================================================
static int GetCVarInt(CVar* cvar, int fallback = 0)
{
    if (!cvar || !cvar->m_str) return fallback;
    return atoi(cvar->m_str);
}
static void SetCVarInt(CVar* cvar, int value)
{
    if (!cvar) return;
    std::string s = std::to_string(value);
    cvar->SetValue(s.c_str(), 0, 0, 0, 0);
}

// ============================================================================
// IDs & Globals
// ============================================================================
static std::atomic<int> g_nextUtteranceID{1};
static int GetNextUtteranceID() {
    int v = g_nextUtteranceID.fetch_add(1, std::memory_order_relaxed);
    if (v == (std::numeric_limits<int>::max)()) {
        g_nextUtteranceID.store(1, std::memory_order_relaxed);
        return 1;
    }
    return v;
}

static std::vector<VoiceTtsVoiceType> g_cachedVoices;

static CVar* s_cvar_voiceID;
static CVar* s_cvar_speed;
static CVar* s_cvar_volume;

// Global SAPI voice instance (single threaded apartment)
static ISpVoice* g_pVoice = nullptr;

// streamNum -> (utteranceID, destination)
struct UtteranceMeta {
    int id;
    int destination;           // kept to echo in events
    bool startedEmitted = false;
};
static std::mutex g_streamMx;
static std::unordered_map<ULONG, UtteranceMeta> g_streamMap;

// ============================================================================
// UTF-8 / Wide helpers
// ============================================================================
std::string WideStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeNeeded == 0) return std::string();
    std::string result(sizeNeeded - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], sizeNeeded - 1, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWide(const char* utf8Str)
{
    if (!utf8Str) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, nullptr, 0);
    if (size == 0) return L"";
    std::wstring wide(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wide[0], size);
    return wide;
}

// ============================================================================
// Small Types & Helpers
// ============================================================================
struct VoiceTtsVoiceType {
    int voiceID;
    std::wstring name;
};

// Resolve a voice name by its index (returns UTF-8)
static std::string GetVoiceNameByID(int voiceID)
{
    auto voices = VoiceChat_GetTtsVoices();
    if (voiceID < 0 || voiceID >= (int)voices.size()) return std::string();
    return WideStringToUtf8(voices[voiceID].name);
}

// Case-insensitive wide-string equality
static bool iequals(const std::wstring& a, const std::wstring& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (towlower(a[i]) != towlower(b[i])) return false;
    return true;
}

// ============================================================================
// Lua event helpers (typed, nullptr-safe, id-safe)
// ============================================================================

static inline lua_State* TryGetLua() {
    return Lua::GetLuaState(); // may be null during very early startup
}

static inline int GetEvtIdOrNeg1(const char* evt) {
    return FrameScript::GetEventIdByName(evt); // -1 if not registered yet
}

static inline void FireEvent_NoArgs(const char* evt)
{
    lua_State* L = TryGetLua();
    int id = GetEvtIdOrNeg1(evt);
    if (!L || id < 0) return;

    Lua::lua_pushstring(L, evt);
    FrameScript::FireEvent_inner(id, L, 1);
    Lua::lua_pop(L, 1);
}

static inline void FireEvent_TTS_PlaybackFailed(const char* status, int utteranceID, int dest)
{
    lua_State* L = TryGetLua();
    int id = GetEvtIdOrNeg1(VOICE_CHAT_TTS_PLAYBACK_FAILED);
    if (!L || id < 0) return;

    Lua::lua_pushstring(L, VOICE_CHAT_TTS_PLAYBACK_FAILED);
    Lua::lua_pushstring(L, status);
    Lua::lua_pushnumber(L, utteranceID);
    Lua::lua_pushnumber(L, dest); // number
    FrameScript::FireEvent_inner(id, L, 4);
    Lua::lua_pop(L, 4);
}

static inline void FireEvent_TTS_PlaybackStarted(int numConsumers, int utteranceID, int durationMS, int dest)
{
    lua_State* L = TryGetLua();
    int id = GetEvtIdOrNeg1(VOICE_CHAT_TTS_PLAYBACK_STARTED);
    if (!L || id < 0) return;

    Lua::lua_pushstring(L, VOICE_CHAT_TTS_PLAYBACK_STARTED);
    Lua::lua_pushnumber(L, numConsumers);
    Lua::lua_pushnumber(L, utteranceID);
    Lua::lua_pushnumber(L, durationMS);
    Lua::lua_pushnumber(L, dest); // number
    FrameScript::FireEvent_inner(id, L, 5);
    Lua::lua_pop(L, 5);
}

static inline void FireEvent_TTS_PlaybackFinished(int numConsumers, int utteranceID, int dest)
{
    lua_State* L = TryGetLua();
    int id = GetEvtIdOrNeg1(VOICE_CHAT_TTS_PLAYBACK_FINISHED);
    if (!L || id < 0) return;

    Lua::lua_pushstring(L, VOICE_CHAT_TTS_PLAYBACK_FINISHED);
    Lua::lua_pushnumber(L, numConsumers);
    Lua::lua_pushnumber(L, utteranceID);
    Lua::lua_pushnumber(L, dest); // number
    FrameScript::FireEvent_inner(id, L, 4);
    Lua::lua_pop(L, 4);
}

// ============================================================================
// COM / SAPI Initialization & Event Pump (one-time setup)
// ============================================================================
static void VoiceChat_InitCOM()
{
    static bool comInitDone = false;
    if (!comInitDone) {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr)) {
            comInitDone = true;
        }
    }
}

// ============================================================================
// SAPI helpers
// ============================================================================
static bool VoiceChat_StartSpeakNow(int voiceID, const std::wstring& text, int rate, int volume, ULONG* outStreamNum /*nullable*/)
{
    // Select voice by index
    IEnumSpObjectTokens* pEnum = nullptr; ULONG count = 0;
    HRESULT hr = SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &pEnum);
    bool voiceSet = false;
    if (SUCCEEDED(hr) && pEnum) {
        ISpObjectToken* pToken = nullptr; ULONG index = 0;
        while (pEnum->Next(1, &pToken, &count) == S_OK && count) {
            if (index == static_cast<ULONG>(voiceID)) {
                if (SUCCEEDED(g_pVoice->SetVoice(pToken))) voiceSet = true;
                pToken->Release();
                break;
            }
            pToken->Release(); ++index;
        }
        pEnum->Release();
    }
    if (!voiceSet) return false;

    // Clamp and apply params
    rate   = std::clamp(rate,   -10, 10);
    volume = std::clamp(volume,   0, 100);
    g_pVoice->SetRate(rate);
    g_pVoice->SetVolume(volume);

    ULONG streamNum = 0;
    HRESULT speakResult = g_pVoice->Speak(text.c_str(), SPF_ASYNC, &streamNum);
    if (FAILED(speakResult)) return false;

    if (outStreamNum) *outStreamNum = streamNum;
    return true;
}

// SAPI notifies via a callback "ping"; actual events are drained via GetEvents.
static void __stdcall OnSpeechNotify(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
    if (!g_pVoice) return;

    SPEVENT ev = {};
    ULONG fetched = 0;

    while (SUCCEEDED(g_pVoice->GetEvents(1, &ev, &fetched)) && fetched == 1)
    {
        switch (ev.eEventId)
        {
            case SPEI_START_INPUT_STREAM:
            {
                std::lock_guard<std::mutex> lock(g_streamMx);
                auto it = g_streamMap.find(ev.ulStreamNum);
                if (it != g_streamMap.end() && !it->second.startedEmitted) {
                    UtteranceMeta& meta = it->second;
                    FireEvent_TTS_PlaybackStarted(1, meta.id, /*durationMS*/0, meta.destination);
                    meta.startedEmitted = true;
                }
                break;
            }
            case SPEI_END_INPUT_STREAM:
            {
                UtteranceMeta meta{0, DEST_LOCAL_PLAYBACK};
                {
                    std::lock_guard<std::mutex> lock(g_streamMx);
                    auto it = g_streamMap.find(ev.ulStreamNum);
                    if (it != g_streamMap.end()) {
                        meta = it->second;
                        g_streamMap.erase(it);
                    }
                }
                FireEvent_TTS_PlaybackFinished(1, meta.id, meta.destination);
                break;
            }
        }
        ::SpClearEvent(&ev);
    }
}

void VoiceChat_InitVoice()
{
    if (!g_pVoice) {
        VoiceChat_InitCOM();
        HRESULT hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_ISpVoice, (void**)&g_pVoice);
        if (SUCCEEDED(hr)) {
            // Register the notify callback once; events will be polled via GetEvents.
            g_pVoice->SetNotifyCallbackFunction(OnSpeechNotify, 0, 0);

            ULONGLONG interest =
                SPFEI(SPEI_START_INPUT_STREAM) |
                SPFEI(SPEI_END_INPUT_STREAM);

            g_pVoice->SetInterest(interest, interest);
        }
    }
}

// ============================================================================
// SAPI Voice Utilities (enumeration / stop-all / speak)
// ============================================================================
static std::vector<VoiceTtsVoiceType> VoiceChat_GetTtsVoices()
{
    VoiceChat_InitCOM();

    std::vector<VoiceTtsVoiceType> voices;
    IEnumSpObjectTokens* pEnum = nullptr;
    ULONG count = 0;

    HRESULT hr = SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &pEnum);
    if (SUCCEEDED(hr) && pEnum)
    {
        ISpObjectToken* pToken = nullptr;
        ULONG index = 0;
        while (pEnum->Next(1, &pToken, &count) == S_OK && count)
        {
            WCHAR* description = nullptr;
            if (SUCCEEDED(SpGetDescription(pToken, &description)) && description)
            {
                voices.push_back({ static_cast<int>(index), description });
                CoTaskMemFree(description);
            }
            pToken->Release();
            ++index;
        }
        pEnum->Release();
    }

    return voices;
}

static std::vector<VoiceTtsVoiceType> VoiceChat_GetRemoteTtsVoices()
{
    // Placeholder: currently identical to local TTS voices.
    return VoiceChat_GetTtsVoices();
}

static void VoiceChat_RefreshVoices()
{
    auto newVoices = VoiceChat_GetTtsVoices();

    // Compare with cache
    bool changed = false;
    if (newVoices.size() != g_cachedVoices.size()) {
        changed = true;
    } else {
        for (size_t i = 0; i < newVoices.size(); ++i) {
            if (newVoices[i].name != g_cachedVoices[i].name) {
                changed = true;
                break;
            }
        }
    }

    g_cachedVoices = std::move(newVoices);

    if (changed) {
        FireEvent_NoArgs(VOICE_CHAT_TTS_VOICES_UPDATE);
    }
}

static void VoiceChat_StopAll()
{
    VoiceChat_InitVoice();
    if (!g_pVoice) return;

    // Stop and purge all queued/ongoing speech in SAPI
    g_pVoice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);

    // Drop all pending stream metadata
    {
        std::lock_guard<std::mutex> lock(g_streamMx);
        g_streamMap.clear();
    }
}

// Speak text: directly hand off to SAPI (no app-side queuing).
static void VoiceChat_SpeakText(int voiceID, const std::wstring& text, int destination, int rate, int volume)
{
    const int utteranceID = GetNextUtteranceID();

    VoiceChat_InitVoice();
    if (!g_pVoice) {
        FireEvent_TTS_PlaybackFailed("EngineAllocationFailed", utteranceID, ClampDestination(destination));
        return;
    }

    const int dest = ClampDestination(destination);

    ULONG streamNum = 0;
    if (!VoiceChat_StartSpeakNow(voiceID, text, rate, volume, &streamNum)) {
        FireEvent_TTS_PlaybackFailed("InternalError", utteranceID, dest);
        return;
    }

    // Track stream for START/FINISH events (dest echoed back)
    {
        std::lock_guard<std::mutex> lock(g_streamMx);
        g_streamMap[streamNum] = UtteranceMeta{ utteranceID, dest, /*startedEmitted*/false };
    }
}

// Convenience overload (CVars), uses destination=1
void VoiceChat_SpeakText(const std::wstring& text)
{
    int voiceID = atoi(s_cvar_voiceID->m_str);
    int speed   = atoi(s_cvar_speed->m_str);
    int volume  = atoi(s_cvar_volume->m_str);
    VoiceChat_SpeakText(voiceID, text, DEST_LOCAL_PLAYBACK, speed, volume);
}

// ============================================================================
// Lua Bindings – C_VoiceChat (enumeration / speak / stop-all)
// ============================================================================
static int Lua_VoiceChat_GetTtsVoices(lua_State* L)
{
    // ensure voices are up-to-date (fires VOICES_UPDATE if changed)
    VoiceChat_RefreshVoices();

    // then return the cached voices list
    Lua::lua_createtable(L, 0, 0);

    int i = 1;
    for (const auto& voice : g_cachedVoices)
    {
        Lua::lua_newtable(L);

        Lua::lua_pushnumber(L, voice.voiceID);
        Lua::lua_setfield(L, -2, "voiceID");

        std::string nameUtf8 = WideStringToUtf8(voice.name);
        Lua::lua_pushstring(L, nameUtf8.c_str());
        Lua::lua_setfield(L, -2, "name");

        Lua::lua_rawseti(L, -2, i++);
    }
    return 1;
}

static int Lua_VoiceChat_GetRemoteTtsVoices(lua_State* L)
{
    auto voices = VoiceChat_GetTtsVoices();
    Lua::lua_createtable(L, 0, 0);

    int i = 1;
    for (const auto& voice : voices)
    {
        Lua::lua_newtable(L);

        Lua::lua_pushnumber(L, voice.voiceID);
        Lua::lua_setfield(L, -2, "voiceID");

        std::string nameUtf8 = WideStringToUtf8(voice.name);
        Lua::lua_pushstring(L, nameUtf8.c_str());
        Lua::lua_setfield(L, -2, "name");

        Lua::lua_rawseti(L, -2, i++);
    }
    return 1;
}

static int Lua_VoiceChat_SpeakText(lua_State* L)
{
    int voiceID = Lua::luaL_checknumber(L, 1);
    const char* text = Lua::luaL_checklstring(L, 2, nullptr);

    int dest = DEST_LOCAL_PLAYBACK; // default
    if (Lua::lua_gettop(L) >= 3 && !Lua::lua_isnil(L, 3)) {
        dest = Lua::luaL_checknumber(L, 3); // only number
        dest = (dest == DEST_QUEUED_LOCAL_PLAYBACK) ? DEST_QUEUED_LOCAL_PLAYBACK : DEST_LOCAL_PLAYBACK;
    }

    int rate = 0;
    if (Lua::lua_gettop(L) >= 4 && !Lua::lua_isnil(L, 4))
        rate = Lua::luaL_checknumber(L, 4);

    int volume = 100;
    if (Lua::lua_gettop(L) >= 5 && !Lua::lua_isnil(L, 5))
        volume = Lua::luaL_checknumber(L, 5);

    std::wstring wText = Utf8ToWide(text);
    VoiceChat_SpeakText(voiceID, wText, dest, rate, volume);
    return 0;
}

static int Lua_VoiceChat_StopSpeakingText(lua_State* L)
{
    VoiceChat_StopAll();
    return 0;
}

// Register C_VoiceChat global
static int lua_openlibvoicechat(lua_State* L)
{
    Lua::luaL_Reg methods[] = {
        {"GetTtsVoices",       Lua_VoiceChat_GetTtsVoices},
        {"GetRemoteTtsVoices", Lua_VoiceChat_GetRemoteTtsVoices},
        {"SpeakText",          Lua_VoiceChat_SpeakText},
        {"StopSpeakingText",   Lua_VoiceChat_StopSpeakingText}
    };

    Lua::lua_createtable(L, 0, std::size(methods));
    for (size_t i = 0; i < std::size(methods); i++) {
        Lua::lua_pushcfunction(L, methods[i].func);
        Lua::lua_setfield(L, -2, methods[i].name);
    }
    Lua::lua_setglobal(L, "C_VoiceChat");
    return 0;
}

static int Lua_TTS_RefreshVoices(lua_State* L)
{
    VoiceChat_RefreshVoices();
    return 0;
}

// ============================================================================
// Lua Bindings – C_TTSSettings (getters / setters)
// ============================================================================

// -- Setters --
static int Lua_TTS_SetDefaultSettings(lua_State* L)
{
    // Blizzard-like defaults: voice=1 (if available), rate=0, volume=100
    int maxVoice = (int)VoiceChat_GetTtsVoices().size();
    int voiceID  = (maxVoice > 1) ? 1 : 0;

    SetCVarInt(s_cvar_voiceID, voiceID);
    SetCVarInt(s_cvar_speed,   0);
    SetCVarInt(s_cvar_volume,  100);

    FireEvent_NoArgs(VOICE_CHAT_TTS_VOICES_UPDATE);
    return 0;
}

static int Lua_TTS_SetSpeechRate(lua_State* L)
{
    int v = Lua::luaL_checknumber(L, 1);
    v = std::clamp(v, -10, 10);
    SetCVarInt(s_cvar_speed, v);
    return 0;
}

static int Lua_TTS_SetSpeechVolume(lua_State* L)
{
    int v = Lua::luaL_checknumber(L, 1);
    v = std::clamp(v, 0, 100);
    SetCVarInt(s_cvar_volume, v);
    return 0;
}

// SetVoiceOption(voiceID:number)
static int Lua_TTS_SetVoiceOptionByID(lua_State* L)
{
    int id = Lua::luaL_checknumber(L, 1);
    int maxVoice = static_cast<uint32_t>(VoiceChat_GetTtsVoices().size());
    if (maxVoice == 0) return 0;

    id = std::clamp(id, 0, maxVoice - 1);
    SetCVarInt(s_cvar_voiceID, id);

    FireEvent_NoArgs(VOICE_CHAT_TTS_VOICES_UPDATE);
    return 0;
}

// SetVoiceOptionByName(voiceName:string)
static int Lua_TTS_SetVoiceOptionByName(lua_State* L)
{
    const char* nameUtf8 = Lua::luaL_checklstring(L, 1, nullptr);
    std::wstring wname = Utf8ToWide(nameUtf8);

    auto voices = VoiceChat_GetTtsVoices();
    int found = -1;
    for (size_t i = 0; i < voices.size(); ++i) {
        if (iequals(voices[i].name, wname)) { found = (int)i; break; }
    }
    if (found >= 0) {
        SetCVarInt(s_cvar_voiceID, found);
        FireEvent_NoArgs(VOICE_CHAT_TTS_VOICES_UPDATE);
    }
    return 0;
}

// -- Getters --
static int Lua_TTS_GetSpeechRate(lua_State* L)
{
    Lua::lua_pushnumber(L, GetCVarInt(s_cvar_speed, 0));
    return 1;
}

static int Lua_TTS_GetSpeechVolume(lua_State* L)
{
    Lua::lua_pushnumber(L, GetCVarInt(s_cvar_volume, 100));
    return 1;
}

static int Lua_TTS_GetSpeechVoiceID(lua_State* L)
{
    Lua::lua_pushnumber(L, GetCVarInt(s_cvar_voiceID, 0));
    return 1;
}

static int Lua_TTS_GetVoiceOptionName(lua_State* L)
{
    int id = GetCVarInt(s_cvar_voiceID, 0);
    std::string name = GetVoiceNameByID(id);
    Lua::lua_pushstring(L, name.c_str());
    return 1;
}

// Register C_TTSSettings global
static int lua_openlibttssettings(lua_State* L)
{
    Lua::lua_createtable(L, 0, 0);

    // Getters
    Lua::lua_pushcfunction(L, Lua_TTS_GetSpeechRate);      Lua::lua_setfield(L, -2, "GetSpeechRate");
    Lua::lua_pushcfunction(L, Lua_TTS_GetSpeechVolume);    Lua::lua_setfield(L, -2, "GetSpeechVolume");
    Lua::lua_pushcfunction(L, Lua_TTS_GetSpeechVoiceID);   Lua::lua_setfield(L, -2, "GetSpeechVoiceID");
    Lua::lua_pushcfunction(L, Lua_TTS_GetVoiceOptionName); Lua::lua_setfield(L, -2, "GetVoiceOptionName");

    // Setters
    Lua::lua_pushcfunction(L, Lua_TTS_SetDefaultSettings);      Lua::lua_setfield(L, -2, "SetDefaultSettings");
    Lua::lua_pushcfunction(L, Lua_TTS_SetSpeechRate);           Lua::lua_setfield(L, -2, "SetSpeechRate");
    Lua::lua_pushcfunction(L, Lua_TTS_SetSpeechVolume);         Lua::lua_setfield(L, -2, "SetSpeechVolume");
    Lua::lua_pushcfunction(L, Lua_TTS_SetVoiceOptionByID);      Lua::lua_setfield(L, -2, "SetVoiceOption");       // by ID
    Lua::lua_pushcfunction(L, Lua_TTS_SetVoiceOptionByName);    Lua::lua_setfield(L, -2, "SetVoiceOptionByName"); // by name
    
    // Refresh
    Lua::lua_pushcfunction(L, Lua_TTS_RefreshVoices);
    Lua::lua_setfield(L, -2, "RefreshVoices");

    Lua::lua_setglobal(L, "C_TTSSettings");
    return 0;
}

// ============================================================================
// CVar Change Callbacks (clamping & sanity)
// ============================================================================
static int OnVoiceIDChanged(CVar* cvar, const char* prevVal, const char* newVal, void* udata)
{
    int val = atoi(newVal);
    int maxVoice = static_cast<int>(VoiceChat_GetTtsVoices().size()) - 1;
    if (val < 0) val = 0;
    if (val > maxVoice) val = maxVoice;
    std::string valStr = std::to_string(val);
    cvar->SetValue(valStr.c_str(), 0, 0, 0, 0);
    return 1;
}

static int OnSpeedChanged(CVar* cvar, const char* prevVal, const char* newVal, void* udata)
{
    int val = atoi(newVal);
    if (val < -10) val = -10;
    else if (val > 10) val = 10;
    std::string valStr = std::to_string(val);
    cvar->SetValue(valStr.c_str(), 0, 0, 0, 0);
    return 1;
}

static int OnVolumeChanged(CVar* cvar, const char* prevVal, const char* newVal, void* udata)
{
    int val = atoi(newVal);
    if (val < 0) val = 0;
    else if (val > 100) val = 100;
    std::string valStr = std::to_string(val);
    cvar->SetValue(valStr.c_str(), 0, 0, 0, 0);
    return 1;
}

// Register CVars used by TTS across sessions
void RegisterVoiceChatCVars()
{
    Hooks::FrameXML::registerCVar(&s_cvar_voiceID, "ttsVoice",  nullptr, "1",  OnVoiceIDChanged); // Blizzard default 1 (English)
    Hooks::FrameXML::registerCVar(&s_cvar_speed,   "ttsSpeed",  nullptr, "0",  OnSpeedChanged);   // Default 0 (normal)
    Hooks::FrameXML::registerCVar(&s_cvar_volume,  "ttsVolume", nullptr, "100",OnVolumeChanged);  // Default 100 (full)
}

using CGameDestroy_t = int(__cdecl*)();
auto CGame_Destroy_orig = reinterpret_cast<CGameDestroy_t>(0x00406B70);

int __cdecl CGame_DestroyHk() {
    VoiceChat::shutdown();
    return CGame_Destroy_orig();
}

// ============================================================================
// Library Registration & Initialization
// ============================================================================
void VoiceChat::initialize()
{
    VoiceChat_InitVoice();
    RegisterVoiceChatCVars();

    Hooks::FrameXML::registerLuaLib(lua_openlibvoicechat);
    Hooks::FrameXML::registerLuaLib(lua_openlibttssettings);

    Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_PLAYBACK_FAILED);
    Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_PLAYBACK_FINISHED);
    Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_PLAYBACK_STARTED);
    Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE); // unused
    Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_VOICES_UPDATE);

    Hooks::Detour(&CGame_Destroy_orig, CGame_DestroyHk);

    VoiceChat_RefreshVoices(); // fill cache and fire VOICES_UPDATE at startup
}

void VoiceChat::shutdown()
{
    if (!g_pVoice) {
        return;
    }

    VoiceChat_StopAll();
    g_pVoice->Release();
    g_pVoice = nullptr;
}
