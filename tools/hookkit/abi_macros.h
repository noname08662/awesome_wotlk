#pragma once

#ifdef _MSVC_LANG
static_assert(_MSVC_LANG >= 202002L, "hookkit: requires C++20 (pass /std:c++20 or later)");
#else
static_assert(__cplusplus >= 202002L, "hookkit: requires C++20 (pass -std=c++20 or later)");
#endif

static_assert(sizeof(void*) == 4,
    "hookkit: this split (WildHook/Desc<eThiscall> ABI emulation, trampoline.h's x86 JIT) is x86-only; "
    "x64 silently mis-marshals arguments instead of failing loudly. Remove this assert only after "
    "restoring an x64-correct ABI path.");

#define HOOKKIT_CDECL __cdecl
#define HOOKKIT_STDCALL __stdcall
#define HOOKKIT_FASTCALL __fastcall
#define HOOKKIT_VECTORCALL __vectorcall

#define HOOKKIT_FORCEINLINE __forceinline

#if defined(__clang__)
#define HOOKKIT_INLINE_CALL [[clang::always_inline]]
#elif defined(_MSC_VER)
#define HOOKKIT_INLINE_CALL [[msvc::forceinline_calls]]
#else
#define HOOKKIT_INLINE_CALL
#endif
