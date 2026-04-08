#pragma once
#include "GameClient.h"
#include "Lua.h"
#include <Detours/detours.h>

namespace Hooks {
	template <typename addr, typename detour>
	inline LONG Detour(addr** ppPointer, detour pDetour) {
		return DetourAttach(reinterpret_cast<PVOID*>(ppPointer), reinterpret_cast<PVOID>(pDetour));
	}
	template <typename addr, typename detour>
	inline LONG Detach(addr** ppPointer, detour pDetour) {
		return DetourDetach(reinterpret_cast<PVOID*>(ppPointer), reinterpret_cast<PVOID>(pDetour));
	}

	namespace FrameScript {
		using namespace ::FrameScript;

		using TokenGuidGetter = guid_t();
		using TokenNGuidGetter = guid_t(int);
		using TokenIdGetter = bool(guid_t);
		using TokenIdNGetter = int(guid_t);

		// Alone tokens like player, target, focus
		void registerToken(const char* token, TokenGuidGetter* getGuid, TokenIdGetter* getId);
		// One more tokens like party1, raid1, arena1
		void registerToken(const char* token, TokenNGuidGetter* getGuid, TokenIdNGetter* getId);
		void registerOnUpdate(const FunctionCallback_t& func);
		void registerOnEnter(const FunctionCallback_t& func);
		void registerOnLeave(const FunctionCallback_t& func);
	}

	namespace FrameXML {
		void registerEvent(const char* str);
		void registerCVar(CVar** dst, const char* str, const char* desc, const char* initialValue, CVar::Handler_t func,
			CVar::CVarFlags flags = static_cast<CVar::CVarFlags>(1), const std::function<void(CVar*)>& initCallback = nullptr);
		void registerLuaLib(const Lua::lua_CFunction& func);
	}

	namespace GlueXML {
		void registerPostLoad(const FunctionCallback_t& func);
		void registerCharEnum(const FunctionCallback_t& func);
	}
	void initialize();
}