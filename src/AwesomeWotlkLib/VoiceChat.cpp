#include "VoiceChat.h"
#include "GameClient.h"
#include "Lua.h"
#include "Utils.h"
#include "Hooks.h"
#include <sapi.h>
#include <sphelper.h>
#include <locale>
#include <vector>
#include <string>
#include <windows.h>
#include <limits>
#include <atomic>
#include <mutex>
#include "unordered_dense/include/ankerl/unordered_dense.h"
#include <algorithm>

#undef min
#undef max

#define VOICE_CHAT_TTS_PLAYBACK_FAILED   "VOICE_CHAT_TTS_PLAYBACK_FAILED"
#define VOICE_CHAT_TTS_PLAYBACK_FINISHED "VOICE_CHAT_TTS_PLAYBACK_FINISHED"
#define VOICE_CHAT_TTS_PLAYBACK_STARTED  "VOICE_CHAT_TTS_PLAYBACK_STARTED"
#define VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE "VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE"
#define VOICE_CHAT_TTS_VOICES_UPDATE     "VOICE_CHAT_TTS_VOICES_UPDATE"

enum ETtsEventType : uint32_t {
	TTS_EVENT_STARTED, TTS_EVENT_FINISHED
};

enum EPlaybackDest : uint32_t {
	DEST_LOCAL_PLAYBACK = 1, DEST_QUEUED_LOCAL_PLAYBACK = 4,
};

struct PendingTtsEvent {
	ETtsEventType type;
	int numConsumers;
	int utteranceID;
	int durationMS;
	int destination;
};

struct VoiceTtsVoiceType {
	int voiceID;
	std::wstring name;
	ISpObjectToken* pToken;
};

static std::mutex g_pendingEventsMx;
static std::vector<PendingTtsEvent> g_pendingEvents;
static std::vector<VoiceTtsVoiceType> VoiceChat_GetTtsVoices();

static std::atomic g_nextUtteranceID{1};

static int GetNextUtteranceID() {
	int v = g_nextUtteranceID.fetch_add(1, std::memory_order_relaxed);
	if (v == (std::numeric_limits<int>::max)()) {
		g_nextUtteranceID.store(1, std::memory_order_relaxed);
		return 1;
	}
	return v;
}

static std::vector<VoiceTtsVoiceType> g_cachedVoices;
static bool g_needsComCleanup = false;

static CVar* s_cvar_voiceID;
static CVar* s_cvar_speed;
static CVar* s_cvar_volume;

static int g_voiceId;
static int g_voiceSpeed;
static int g_voiceVolume;

static ISpVoice* g_pVoice = nullptr;

struct UtteranceMeta {
	int id;
	int destination;
	bool startedEmitted = false;
};

static std::mutex g_streamMx;
static ankerl::unordered_dense::map<ULONG, UtteranceMeta> g_streamMap;

static std::string GetVoiceNameByID(int voiceID) {
	if (voiceID < 0 || voiceID >= std::ssize(g_cachedVoices)) return std::string();
	return WideToUtf8(g_cachedVoices[voiceID].name);
}

static void FireEvent_NoArgs(const char* evt) {
	lua_State* L = Lua::GetLuaState();
	int id = FrameScript::GetEventIdByName(evt);
	if (!L || id < 0) return;

	Lua::lua_pushstring(L, evt);
	FrameScript::FireEvent_inner(id, L, 1);
	Lua::lua_pop(L, 1);
}

static void FireEvent_TTS_PlaybackFailed(const char* status, int utteranceID, int dest) {
	lua_State* L = Lua::GetLuaState();
	int id = FrameScript::GetEventIdByName(VOICE_CHAT_TTS_PLAYBACK_FAILED);
	if (!L || id < 0) return;

	Lua::lua_pushstring(L, VOICE_CHAT_TTS_PLAYBACK_FAILED);
	Lua::lua_pushstring(L, status);
	Lua::lua_pushnumber(L, utteranceID);
	Lua::lua_pushnumber(L, dest);
	FrameScript::FireEvent_inner(id, L, 4);
	Lua::lua_pop(L, 4);
}

static void FireEvent_TTS_PlaybackStarted(int numConsumers, int utteranceID, int durationMS, int dest) {
	lua_State* L = Lua::GetLuaState();
	int id = FrameScript::GetEventIdByName(VOICE_CHAT_TTS_PLAYBACK_STARTED);
	if (!L || id < 0) return;

	Lua::lua_pushstring(L, VOICE_CHAT_TTS_PLAYBACK_STARTED);
	Lua::lua_pushnumber(L, numConsumers);
	Lua::lua_pushnumber(L, utteranceID);
	Lua::lua_pushnumber(L, durationMS);
	Lua::lua_pushnumber(L, dest);
	FrameScript::FireEvent_inner(id, L, 5);
	Lua::lua_pop(L, 5);
}

static void FireEvent_TTS_PlaybackFinished(int numConsumers, int utteranceID, int dest) {
	lua_State* L = Lua::GetLuaState();
	int id = FrameScript::GetEventIdByName(VOICE_CHAT_TTS_PLAYBACK_FINISHED);
	if (!L || id < 0) return;

	Lua::lua_pushstring(L, VOICE_CHAT_TTS_PLAYBACK_FINISHED);
	Lua::lua_pushnumber(L, numConsumers);
	Lua::lua_pushnumber(L, utteranceID);
	Lua::lua_pushnumber(L, dest);
	FrameScript::FireEvent_inner(id, L, 4);
	Lua::lua_pop(L, 4);
}

static ULONG VoiceChat_StartSpeakNow(int voiceID, const std::wstring& text, int rate, USHORT volume) {
	if (!g_pVoice) return 0;
	if (voiceID < 0 || voiceID >= std::ssize(g_cachedVoices)) return 0;

	ISpObjectToken* pToken = g_cachedVoices[voiceID].pToken;
	if (!pToken || FAILED(g_pVoice->SetVoice(pToken))) return 0;

	rate = std::clamp(rate, -10, 10);
	volume = std::clamp(static_cast<int>(volume), 0, 100);
	g_pVoice->SetRate(rate);
	g_pVoice->SetVolume(volume);

	ULONG streamNum = 0;
	HRESULT speakResult = g_pVoice->Speak(text.c_str(), SPF_ASYNC, &streamNum);
	if (FAILED(speakResult)) return 0;

	return streamNum;
}

static std::vector<VoiceTtsVoiceType> VoiceChat_GetTtsVoices() {
	std::vector<VoiceTtsVoiceType> voices;
	IEnumSpObjectTokens* pEnum = nullptr;
	ULONG count = 0;

	HRESULT hr = SpEnumTokens(SPCAT_VOICES, nullptr, nullptr, &pEnum);
	if (SUCCEEDED(hr) && pEnum) {
		ISpObjectToken* pToken = nullptr;
		ULONG index = 0;
		while (pEnum->Next(1, &pToken, &count) == S_OK && count) {
			WCHAR* description = nullptr;
			if (SUCCEEDED(SpGetDescription(pToken, &description)) && description) {
				pToken->AddRef();
				voices.push_back({static_cast<int>(index), description, pToken});
				CoTaskMemFree(description);
			}
			pToken->Release();
			++index;
		}
		pEnum->Release();
	}
	return voices;
}

static void VoiceChat_RefreshVoices() {
	auto newVoices = VoiceChat_GetTtsVoices();
	bool changed = false;
	if (newVoices.size() != g_cachedVoices.size()) { changed = true; }
	else {
		for (size_t i = 0; i < newVoices.size(); ++i) {
			if (newVoices[i].name != g_cachedVoices[i].name) {
				changed = true;
				break;
			}
		}
	}

	if (changed) {
		for (auto& v : g_cachedVoices) { if (v.pToken) v.pToken->Release(); }
		g_cachedVoices = std::move(newVoices);
		FireEvent_NoArgs(VOICE_CHAT_TTS_VOICES_UPDATE);
	}
	else { for (auto& v : newVoices) { if (v.pToken) v.pToken->Release(); } }
}

static void VoiceChat_StopAll() {
	if (!g_pVoice) return;
	g_pVoice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
}

static void VoiceChat_StopAll_Async() {
	if (!g_pVoice) return;
	ISpVoice* pLocalVoice = g_pVoice;
	pLocalVoice->AddRef();
	std::thread([pLocalVoice]() {
		HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		pLocalVoice->Speak(nullptr, SPF_PURGEBEFORESPEAK, nullptr);
		pLocalVoice->Release();
		if (SUCCEEDED(hr)) CoUninitialize();
	}).detach();
}

static void VoiceChat_SpeakText(int voiceID, const std::wstring& text, int destination, int rate, int volume) {
	std::scoped_lock lock(g_streamMx);

	const int utteranceID = GetNextUtteranceID();
	const int dest = (destination == DEST_QUEUED_LOCAL_PLAYBACK) ? DEST_QUEUED_LOCAL_PLAYBACK : DEST_LOCAL_PLAYBACK;

	ULONG streamNum = VoiceChat_StartSpeakNow(voiceID, text, rate, static_cast<USHORT>(volume));
	if (streamNum == 0) {
		FireEvent_TTS_PlaybackFailed("InternalError", utteranceID, dest);
		return;
	}
	g_streamMap[streamNum] = UtteranceMeta{utteranceID, dest, false};
}

void VoiceChat_SpeakText(const std::wstring& text) {
	int voiceID = std::atoi(s_cvar_voiceID->m_str);
	int speed = std::atoi(s_cvar_speed->m_str);
	int volume = std::atoi(s_cvar_volume->m_str);
	VoiceChat_SpeakText(voiceID, text, DEST_LOCAL_PLAYBACK, speed, volume);
}

static void VoiceChat_OnUpdate() {
	static std::vector<PendingTtsEvent> s_localEvents;
	{
		std::scoped_lock lock(g_pendingEventsMx);
		if (!g_pendingEvents.empty()) {
			s_localEvents = std::move(g_pendingEvents);
			g_pendingEvents.clear();
		}
	}
	if (!s_localEvents.empty()) {
		for (const auto& ev : s_localEvents) {
			if (ev.type == TTS_EVENT_STARTED) { FireEvent_TTS_PlaybackStarted(ev.numConsumers, ev.utteranceID, ev.durationMS, ev.destination); }
			else if (ev.type == TTS_EVENT_FINISHED) { FireEvent_TTS_PlaybackFinished(ev.numConsumers, ev.utteranceID, ev.destination); }
		}
		s_localEvents.clear();
	}
}

static int Lua_VoiceChat_GetTtsVoices(lua_State* L) {
	VoiceChat_RefreshVoices();

	Lua::lua_createtable(L, 0, 0);

	int i = 1;
	for (const auto& voice : g_cachedVoices) {
		Lua::lua_newtable(L);

		Lua::lua_pushnumber(L, voice.voiceID);
		Lua::lua_setfield(L, -2, "voiceID");

		std::string nameUtf8 = WideToUtf8(voice.name);
		Lua::lua_pushstring(L, nameUtf8.c_str());
		Lua::lua_setfield(L, -2, "name");

		Lua::lua_rawseti(L, -2, i++);
	}
	return 1;
}

static int Lua_VoiceChat_GetRemoteTtsVoices(lua_State* L) {
	auto voices = VoiceChat_GetTtsVoices();
	Lua::lua_createtable(L, 0, 0);

	int i = 1;
	for (const auto& voice : voices) {
		Lua::lua_newtable(L);

		Lua::lua_pushnumber(L, voice.voiceID);
		Lua::lua_setfield(L, -2, "voiceID");

		std::string nameUtf8 = WideToUtf8(voice.name);
		Lua::lua_pushstring(L, nameUtf8.c_str());
		Lua::lua_setfield(L, -2, "name");

		if (voice.pToken) voice.pToken->Release();

		Lua::lua_rawseti(L, -2, i++);
	}
	return 1;
}

static int Lua_VoiceChat_SpeakText(lua_State* L) {
	int voiceID = Lua::luaL_checknumber(L, 1);
	const char* text = Lua::luaL_checklstring(L, 2, nullptr);

	int dest = DEST_LOCAL_PLAYBACK;
	if (Lua::lua_gettop(L) >= 3 && !Lua::lua_isnil(L, 3)) {
		dest = Lua::luaL_checknumber(L, 3);
		dest = (dest == DEST_QUEUED_LOCAL_PLAYBACK) ? DEST_QUEUED_LOCAL_PLAYBACK : DEST_LOCAL_PLAYBACK;
	}

	int rate = 0;
	if (Lua::lua_gettop(L) >= 4 && !Lua::lua_isnil(L, 4)) rate = Lua::luaL_checknumber(L, 4);

	int volume = 100;
	if (Lua::lua_gettop(L) >= 5 && !Lua::lua_isnil(L, 5)) volume = Lua::luaL_checknumber(L, 5);

	VoiceChat_SpeakText(voiceID, Utf8ToWide(text), dest, rate, volume);
	return 0;
}

static int Lua_VoiceChat_StopSpeakingText(lua_State*) {
	VoiceChat_StopAll_Async();
	return 0;
}

static int lua_openlibvoicechat(lua_State* L) {
	Lua::luaL_Reg methods[] = {{"GetTtsVoices", Lua_VoiceChat_GetTtsVoices}, {"GetRemoteTtsVoices", Lua_VoiceChat_GetRemoteTtsVoices}, {"SpeakText", Lua_VoiceChat_SpeakText}, {"StopSpeakingText", Lua_VoiceChat_StopSpeakingText}};
	Lua::lua_createtable(L, 0, std::size(methods));
	for (const auto& method : methods) {
		Lua::lua_pushcfunction(L, method.func);
		Lua::lua_setfield(L, -2, method.name);
	}
	Lua::lua_setglobal(L, "C_VoiceChat");
	return 0;
}

static int Lua_TTS_RefreshVoices(lua_State*) {
	VoiceChat_RefreshVoices();
	return 0;
}

static int Lua_TTS_SetDefaultSettings(lua_State*) {
	int defaultVoice = (static_cast<int>(VoiceChat_GetTtsVoices().size()) > 1) ? 1 : 0;
	s_cvar_voiceID->Sync(std::to_string(defaultVoice).c_str(), &g_voiceId, 0, static_cast<int>(VoiceChat_GetTtsVoices().size()) - 1, "%d");
	s_cvar_speed->Sync("0", &g_voiceSpeed, -10, 10, "%d");
	s_cvar_volume->Sync("100", &g_voiceVolume, 0, 100, "%d");
	FireEvent_NoArgs(VOICE_CHAT_TTS_VOICES_UPDATE);
	return 0;
}

static int Lua_TTS_SetSpeechRate(lua_State* L) {
	s_cvar_speed->Sync(Lua::luaL_checkstring(L, 1), &g_voiceSpeed, -10, 10, "%d");
	return 0;
}

static int Lua_TTS_SetSpeechVolume(lua_State* L) {
	s_cvar_volume->Sync(Lua::luaL_checkstring(L, 1), &g_voiceVolume, 0, 100, "%d");
	return 0;
}

static int Lua_TTS_SetVoiceOptionByID(lua_State* L) {
	int maxVoiceIndex = static_cast<int>(VoiceChat_GetTtsVoices().size()) - 1;
	if (maxVoiceIndex >= 0) {
		s_cvar_voiceID->Sync(Lua::luaL_checkstring(L, 1), &g_voiceId, 0, maxVoiceIndex, "%d");
		FireEvent_NoArgs(VOICE_CHAT_TTS_VOICES_UPDATE);
	}
	return 0;
}

static int Lua_TTS_SetVoiceOptionByName(lua_State* L) {
	const char* nameUtf8 = Lua::luaL_checkstring(L, 1);
	std::wstring wname = Utf8ToWide(nameUtf8);

	auto voices = VoiceChat_GetTtsVoices();
	int found = -1;
	for (size_t i = 0; i < voices.size(); ++i) {
		if (iequals(voices[i].name, wname)) {
			found = static_cast<int>(i);
			break;
		}
		if (voices[i].pToken) voices[i].pToken->Release();
	}
	if (found >= 0) {
		std::string foundStr = std::to_string(found);
		s_cvar_voiceID->Sync(foundStr.c_str(), &g_voiceId, 0, static_cast<int>(voices.size()) - 1, "%d");
		FireEvent_NoArgs(VOICE_CHAT_TTS_VOICES_UPDATE);
	}
	return 0;
}

static int Lua_TTS_GetSpeechRate(lua_State* L) {
	Lua::lua_pushnumber(L, g_voiceSpeed);
	return 1;
}

static int Lua_TTS_GetSpeechVolume(lua_State* L) {
	Lua::lua_pushnumber(L, g_voiceVolume);
	return 1;
}

static int Lua_TTS_GetSpeechVoiceID(lua_State* L) {
	Lua::lua_pushnumber(L, g_voiceId);
	return 1;
}

static int Lua_TTS_GetVoiceOptionName(lua_State* L) {
	Lua::lua_pushstring(L, GetVoiceNameByID(g_voiceId).c_str());
	return 1;
}

static int lua_openlibttssettings(lua_State* L) {
	Lua::lua_createtable(L, 0, 0);

	Lua::lua_pushcfunction(L, Lua_TTS_GetSpeechRate);
	Lua::lua_setfield(L, -2, "GetSpeechRate");
	Lua::lua_pushcfunction(L, Lua_TTS_GetSpeechVolume);
	Lua::lua_setfield(L, -2, "GetSpeechVolume");
	Lua::lua_pushcfunction(L, Lua_TTS_GetSpeechVoiceID);
	Lua::lua_setfield(L, -2, "GetSpeechVoiceID");
	Lua::lua_pushcfunction(L, Lua_TTS_GetVoiceOptionName);
	Lua::lua_setfield(L, -2, "GetVoiceOptionName");

	Lua::lua_pushcfunction(L, Lua_TTS_SetDefaultSettings);
	Lua::lua_setfield(L, -2, "SetDefaultSettings");
	Lua::lua_pushcfunction(L, Lua_TTS_SetSpeechRate);
	Lua::lua_setfield(L, -2, "SetSpeechRate");
	Lua::lua_pushcfunction(L, Lua_TTS_SetSpeechVolume);
	Lua::lua_setfield(L, -2, "SetSpeechVolume");
	Lua::lua_pushcfunction(L, Lua_TTS_SetVoiceOptionByID);
	Lua::lua_setfield(L, -2, "SetVoiceOption");
	Lua::lua_pushcfunction(L, Lua_TTS_SetVoiceOptionByName);
	Lua::lua_setfield(L, -2, "SetVoiceOptionByName");

	Lua::lua_pushcfunction(L, Lua_TTS_RefreshVoices);
	Lua::lua_setfield(L, -2, "RefreshVoices");

	Lua::lua_setglobal(L, "C_TTSSettings");
	return 0;
}

static int OnVoiceIDChanged(CVar* cvar, const char*, const char* newVal, void*) { return cvar->Sync(newVal, &g_voiceId, 0, std::max(static_cast<int>(VoiceChat_GetTtsVoices().size()) - 1, 0), "%d"); }

static int OnSpeedChanged(CVar* cvar, const char*, const char* newVal, void*) { return cvar->Sync(newVal, &g_voiceSpeed, -10, 10, "%d"); }

static int OnVolumeChanged(CVar* cvar, const char*, const char* newVal, void*) { return cvar->Sync(newVal, &g_voiceVolume, 0, 100, "%d"); }

static void __stdcall OnSpeechNotifyCallback(WPARAM, LPARAM) {
	if (!g_pVoice) return;

	SPEVENT ev = {};
	ULONG fetched = 0;

	while (SUCCEEDED(g_pVoice->GetEvents(1, &ev, &fetched)) && fetched == 1) {
		switch (ev.eEventId) {
		case SPEI_START_INPUT_STREAM:
			{
				std::scoped_lock lock(g_streamMx);
				auto it = g_streamMap.find(ev.ulStreamNum);
				if (it != g_streamMap.end() && !it->second.startedEmitted) {
					UtteranceMeta& meta = it->second;
					meta.startedEmitted = true;

					std::scoped_lock evtLock(g_pendingEventsMx);
					g_pendingEvents.push_back({TTS_EVENT_STARTED, 1, meta.id, 0, meta.destination});
				}
				break;
			}
		case SPEI_END_INPUT_STREAM:
			{
				UtteranceMeta meta{0, DEST_LOCAL_PLAYBACK};
				bool metaFound = false;
				{
					std::scoped_lock lock(g_streamMx);
					auto it = g_streamMap.find(ev.ulStreamNum);
					if (it != g_streamMap.end()) {
						meta = it->second;
						g_streamMap.erase(it);
						metaFound = true;
					}
				}
				if (metaFound) {
					std::scoped_lock evtLock(g_pendingEventsMx);
					g_pendingEvents.push_back({TTS_EVENT_FINISHED, 1, meta.id, 0, meta.destination});
				}
				break;
			}
		}
		SpClearEvent(&ev);
	}
}

void VoiceChat::initialize() {
	Hooks::FrameXML::registerCVar(&s_cvar_voiceID, "ttsVoice", nullptr, "1", OnVoiceIDChanged);
	Hooks::FrameXML::registerCVar(&s_cvar_speed, "ttsSpeed", nullptr, "0", OnSpeedChanged);
	Hooks::FrameXML::registerCVar(&s_cvar_volume, "ttsVolume", nullptr, "100", OnVolumeChanged);

	Hooks::FrameXML::registerLuaLib(lua_openlibvoicechat);
	Hooks::FrameXML::registerLuaLib(lua_openlibttssettings);

	Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_PLAYBACK_FAILED);
	Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_PLAYBACK_FINISHED);
	Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_PLAYBACK_STARTED);
	Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_SPEAK_TEXT_UPDATE);
	Hooks::FrameXML::registerEvent(VOICE_CHAT_TTS_VOICES_UPDATE);

	Hooks::FrameScript::registerOnEnter([]() {
		if (!g_pVoice) {
			HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
			if (SUCCEEDED(hrInit)) { g_needsComCleanup = true; }
			HRESULT hr = CoCreateInstance(CLSID_SpVoice, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&g_pVoice));
			if (SUCCEEDED(hr)) {
				g_pVoice->SetNotifyCallbackFunction(OnSpeechNotifyCallback, 0, 0);
				ULONGLONG interest = SPFEI(SPEI_START_INPUT_STREAM) | SPFEI(SPEI_END_INPUT_STREAM);
				g_pVoice->SetInterest(interest, interest);
				VoiceChat_RefreshVoices();
			}
		}
	});

	Hooks::FrameScript::registerOnLeave([]() {
		if (g_pVoice) {
			VoiceChat_StopAll();
			g_pVoice->Release();
			g_pVoice = nullptr;
		}
		{
			std::scoped_lock lock(g_pendingEventsMx);
			g_pendingEvents.clear();
		}

		for (auto& v : g_cachedVoices) { if (v.pToken) v.pToken->Release(); }
		g_cachedVoices.clear();

		if (g_needsComCleanup) {
			CoUninitialize();
			g_needsComCleanup = false;
		}
	});

	Hooks::FrameScript::registerOnUpdate(VoiceChat_OnUpdate);
}
