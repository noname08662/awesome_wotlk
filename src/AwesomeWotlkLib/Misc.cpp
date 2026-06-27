#include "Misc.h"
#include "D3D.h"
#include "GameClient.h"
#include "Hooks.h"
#include "Utils.h"
#include <bit>

namespace {
auto (*PortraitInitialize_site1)() = reinterpret_cast<DummyCallback_t>(0x00616E09);
auto (*PortraitInitialize_site2)() = reinterpret_cast<DummyCallback_t>(0x006180E0);
constexpr uintptr_t PortraitInitialize_site2_jmpback = 0x006180E5;

auto (*PortraitSet_site)() = reinterpret_cast<DummyCallback_t>(0x00619B6A);
constexpr uintptr_t PortraitSet_site_jmpback = 0x00619B72;

bool g_cursorKeywordActive = false;
bool g_playerLocationKeywordActive = false;

enum EObjHLMode : uint32_t {
	HL_DISABLED = 0,
	HL_ALWAYS = 1,
	HL_TRACKED = 2,
};

int g_iAngle = 0;
int g_iMode = 0;
int g_portraitRes = 64;
EObjHLMode g_highlightMode = HL_DISABLED;

CVar* s_cvar_interactionMode;
CVar* s_cvar_interactionAngle;
CVar* s_cvar_objectHighlightMode;
CVar* s_cvar_portraitResolution;
CVar* s_cvar_chatLogSessionKey;
CVar* s_cvar_combatLogSessionKey;

int g_chatLogSessionKey = 1;
int g_combatLogSessionKey = 1;

char g_customChatLogPath[MAX_PATH];
char g_customCombatLogPath[MAX_PATH];

const char* sessionStamp() {
	static const std::string stamp = [] {
		char s[40] = {0};
		SYSTEMTIME t;
		GetLocalTime(&t);
		std::sprintf(s, "%04d-%02d-%02d-%02d.%02d.%02d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
		return std::string(s);
	}();
	return stamp.c_str();
}

const std::vector<uint8_t> validTypes = {GAMEOBJECT_TYPE_DOOR, GAMEOBJECT_TYPE_BUTTON, GAMEOBJECT_TYPE_QUESTGIVER, GAMEOBJECT_TYPE_CHEST, GAMEOBJECT_TYPE_BINDER, GAMEOBJECT_TYPE_CHAIR, GAMEOBJECT_TYPE_SPELL_FOCUS, GAMEOBJECT_TYPE_GOOBER, GAMEOBJECT_TYPE_FISHINGNODE, GAMEOBJECT_TYPE_MAILBOX, GAMEOBJECT_TYPE_MEETINGSTONE, GAMEOBJECT_TYPE_GUILD_BANK};

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

bool IsInteractableGameObject(uint8_t type) { return std::ranges::any_of(validTypes, [type](uint8_t t) { return t == type; }); }

auto isValidObject = [](CGObject_C* object, const CGUnit_C* player) -> bool {
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

		for (char c : modStr) { if (!std::isalnum(static_cast<unsigned char>(c))) return 0; }
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
	else if (guid_t guid = ObjectMgr::GetGuidByUnitID(modifier.c_str())) { trySetCandidate(guid); }
	if (candidate != 0) s_requestedInteraction = candidate;

	return 0;
}

int InteractFunction_C(lua_State* L) {
	const char* param = nullptr;
	if (!Lua::lua_isnoneornil(L, 1)) { param = Lua::lua_tostring(L, 1); }

	Lua::lua_getglobal(L, "SecureCmdOptionParse");
	if (!Lua::lua_isfunction(L, -1)) {
		Lua::lua_pop(L, 1);
		Lua::lua_pushcfunction(L, lua_QueueInteract);
		if (Lua::lua_isfunction(L, -1)) { if (Lua::lua_pcall(L, 0, 0, 0) != 0) { Lua::lua_pop(L, 1); } }
		return 0;
	}

	if (param) Lua::lua_pushstring(L, param);
	else Lua::lua_pushnil(L);

	if (Lua::lua_pcall(L, 1, 2, 0) != 0) {
		Lua::lua_pop(L, 1);
		Lua::lua_pushcfunction(L, lua_QueueInteract);
		if (Lua::lua_isfunction(L, -1)) { if (Lua::lua_pcall(L, 0, 0, 0) != 0) { Lua::lua_pop(L, 1); } }
		return 0;
	}

	if (!Lua::lua_isnil(L, -1)) {
		Lua::lua_pushcfunction(L, lua_QueueInteract);
		if (Lua::lua_isfunction(L, -1)) {
			Lua::lua_pushvalue(L, -2);
			if (Lua::lua_pcall(L, 1, 0, 0) != 0) { Lua::lua_pop(L, 1); }
		}
		Lua::lua_pop(L, 3);
	}
	else {
		Lua::lua_pop(L, 1);
		Lua::lua_pushcfunction(L, lua_QueueInteract);
		if (Lua::lua_isfunction(L, -1)) { if (Lua::lua_pcall(L, 0, 0, 0) != 0) { Lua::lua_pop(L, 1); } }
		Lua::lua_pop(L, 1);
	}
	return 0;
}

char __fastcall CGGameObject_C__CheckForPassiveHighlightHk(CGGameObject_C* pThis) {
	const char result = CGGameObject_C::CheckForPassiveHighlightFn(pThis);
	uint8_t goType = static_cast<uint8_t>((pThis->GetValue<uint32_t>(GAMEOBJECT_BYTES_1) >> 8) & 0xFF);
	if (!pThis->CanUse() || (goType != GAMEOBJECT_TYPE_CHEST && goType != GAMEOBJECT_TYPE_GOOBER && goType != GAMEOBJECT_TYPE_QUESTGIVER)) { return result; }
	if (g_highlightMode == HL_TRACKED) {
		if (goType == GAMEOBJECT_TYPE_QUESTGIVER && pThis->m_questMark == nullptr) return result;
		if (goType == GAMEOBJECT_TYPE_CHEST) {
			if (const LockRec* lockRec = pThis->GetLockRec()) {
				if (lockRec->m_type[0] == 2) return result; // gathering node
			}
		}
	}
	pThis->m_highlightMask |= 0x400000u;
	return CGGameObject_C::ShowLootEffectFn(pThis);
}

int lua_openmisclib(lua_State* L) {
	Lua::luaL_Reg funcs[] = {{"FlashWindow", lua_FlashWindow}, {"IsWindowFocused", lua_IsWindowFocused}, {"FocusWindow", lua_FocusWindow}, {"CopyToClipboard", lua_CopyToClipboard}, {"QueueInteract", lua_QueueInteract}};

	for (const auto& [name, func] : funcs) {
		Lua::lua_pushcfunction(L, func);
		Lua::lua_setglobal(L, name);
	}
	Hooks::FrameScript::registerOnUpdate(ProcessQueuedInteraction);
	return 0;
}

int CVarHandler_interactionAngle(CVar* cvar, const char*, const char* value, void*) { return cvar->Sync(value, &g_iAngle, 15, 160, "%d"); }
int CVarHandler_interactionMode(CVar* cvar, const char*, const char* value, void*) { return cvar->Sync(value, &g_iMode, 0, 1, "%d"); }

int CVarHandler_portraitResolution(CVar* cvar, const char*, const char* value, void*) {
	const int result = cvar->Sync(value, &g_portraitRes, 64, 2048, "%d");
	g_portraitRes = std::bit_ceil(static_cast<unsigned int>(g_portraitRes));
	return result;
}

int CVarHandler_objectHighlightMode(CVar* cvar, const char*, const char* value, void*) {
	const int result = cvar->Sync(value, reinterpret_cast<int*>(&g_highlightMode), static_cast<int>(HL_DISABLED), static_cast<int>(HL_TRACKED), "%d");
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	if (g_highlightMode != HL_DISABLED) Hooks::Detour(&CGGameObject_C::CheckForPassiveHighlightFn, CGGameObject_C__CheckForPassiveHighlightHk);
	else Hooks::Detach(&CGGameObject_C::CheckForPassiveHighlightFn, CGGameObject_C__CheckForPassiveHighlightHk);
	DetourTransactionCommit();
	if (guid_t pg = ObjectMgr::GetPlayerGuid(); ObjectMgr::Get<CGPlayer_C>(pg, TYPEMASK_PLAYER)) {
		ObjectMgr::EnumObjects([&](guid_t guid) -> bool {
			if (guid < 0x1000) return true;

			CGGameObject_C* obj = ObjectMgr::Get<CGGameObject_C>(guid, TYPEMASK_GAMEOBJECT);
			if (!obj || obj->m_typeID != TYPEID_GAMEOBJECT) return true;

			if (g_highlightMode != HL_DISABLED) CGGameObject_C__CheckForPassiveHighlightHk(obj);
			else CGGameObject_C::CheckForPassiveHighlightFn(obj);
			return true;
		});
	}
	return result;
}

int CVarHandler_chatLogSessionKey(CVar* cvar, const char*, const char* value, void*) {
	const int result = cvar->Sync(value, &g_chatLogSessionKey, 0, 1, "%d");
	DWORD oldProtect;
	VirtualProtect(reinterpret_cast<void*>(0x00AC7A40), 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	if (g_chatLogSessionKey) {
		std::sprintf(g_customChatLogPath, "Logs\\%sWoWChatLog.txt", sessionStamp());
		*reinterpret_cast<const char**>(0x00AC7A40) = g_customChatLogPath;
	}
	else { *reinterpret_cast<const char**>(0x00AC7A40) = reinterpret_cast<const char*>(0x009FA4E4); }
	VirtualProtect(reinterpret_cast<void*>(0x00AC7A40), 4, oldProtect, &oldProtect);
	return result;
}

int CVarHandler_combatLogSessionKey(CVar* cvar, const char*, const char* value, void*) {
	const int result = cvar->Sync(value, &g_combatLogSessionKey, 0, 1, "%d");
	DWORD oldProtect;
	VirtualProtect(reinterpret_cast<void*>(0x00AC7A44), 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	if (g_combatLogSessionKey) {
		std::sprintf(g_customCombatLogPath, "Logs\\%sWoWCombatLog.txt", sessionStamp());
		*reinterpret_cast<const char**>(0x00AC7A44) = g_customCombatLogPath;
	}
	else { *reinterpret_cast<const char**>(0x00AC7A44) = reinterpret_cast<const char*>(0x009FA4CC); }
	VirtualProtect(reinterpret_cast<void*>(0x00AC7A44), 4, oldProtect, &oldProtect);
	return result;
}

bool TerrainClick(float x, float y, float z) {
	TerrainClickEvent tc = {.m_guid = 0, .m_pos = {.X = x, .Y = y, .Z = z}, .m_button = 1};
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
		C3Vector cursorPos = {.X = coords[2], .Y = coords[3], .Z = coords[4]};
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

void __declspec(naked) PortraitInitialize_site1Hk() {
	__asm {
		push g_portraitRes;
		push g_portraitRes;
		push esi;
		jmp PortraitInitialize_site2_jmpback;
	}
}

void __declspec(naked) PortraitSet_siteHk() {
	__asm {
		push 2;
		push 2;
		push g_portraitRes;
		push g_portraitRes;
		jmp PortraitSet_site_jmpback;
	}
}

void OnEnterWorld() {
	Lua::RegisterSlashCommand("INTERACTCMD", "/interact", InteractFunction_C);
	Lua::RegisterLuaBinding("AWESOME_KEYBIND", "INTERACTIONKEYBIND", "Interaction Button", "AWESOME_WOTLK_KEYBINDS", "Awesome Wotlk Keybinds", "QueueInteract()");

	if (lua_State* L = Lua::GetLuaState()) {
		char buf[256];
		sprintf(buf, "Chat being logged to %s", *reinterpret_cast<const char**>(0x00AC7A40));
		Lua::lua_pushstring(L, buf);
		Lua::lua_setglobal(L, "CHATLOGENABLED");

		sprintf(buf, "Combat being logged to %s", *reinterpret_cast<const char**>(0x00AC7A44));
		Lua::lua_pushstring(L, buf);
		Lua::lua_setglobal(L, "COMBATLOGENABLED");
	}
}
}

void Misc::initialize() {
	Hooks::FrameXML::registerLuaLib(lua_openmisclib);
	Hooks::FrameXML::registerCVar(&s_cvar_interactionAngle, "interactionAngle", nullptr, "60", CVarHandler_interactionAngle);
	Hooks::FrameXML::registerCVar(&s_cvar_interactionMode, "interactionMode", nullptr, "1", CVarHandler_interactionMode);
	Hooks::FrameXML::registerCVar(&s_cvar_objectHighlightMode, "objectHighlightMode", nullptr, "0", CVarHandler_objectHighlightMode);
	Hooks::FrameXML::registerCVar(&s_cvar_portraitResolution, "portraitResolution", nullptr, "64", CVarHandler_portraitResolution);
	Hooks::FrameXML::registerCVar(&s_cvar_chatLogSessionKey, "chatLogSessionKey", nullptr, "1", CVarHandler_chatLogSessionKey);
	Hooks::FrameXML::registerCVar(&s_cvar_combatLogSessionKey, "combatLogSessionKey", nullptr, "1", CVarHandler_combatLogSessionKey);

	std::uint8_t mov_eax[5] = {0xA1, 0x00, 0x00, 0x00, 0x00};
	uintptr_t varAddress = reinterpret_cast<uintptr_t>(&g_portraitRes);
	std::memcpy(&mov_eax[1], &varAddress, sizeof(varAddress));
	Hooks::PatchBytes(reinterpret_cast<void*>(PortraitInitialize_site1), mov_eax, sizeof(mov_eax));
	Hooks::Detour(&PortraitInitialize_site2, PortraitInitialize_site1Hk);
	Hooks::Detour(&PortraitSet_site, PortraitSet_siteHk);

	Hooks::Detour(&CGGameUI::SecureCmdOptionParseFn, SecureCmdOptionParseHk);
	Hooks::Detour(&CGWorldFrame::OnLayerTrackTerrainFn, OnLayerTrackTerrainHk);
	Hooks::Detour(&SpellCastResetFn, SpellCastResetHk);

	Hooks::FrameScript::registerOnEnter(OnEnterWorld);
}
