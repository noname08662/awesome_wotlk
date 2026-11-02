#pragma once

#include <cctype>
#include <clocale>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "LuaSwapTaint.h"

#include "include/Lib/Storm.h"
#include "include/System/System.h"

// the script profiler's clocks
inline uint64_t OsGetAsyncTimeClocks() { return os::getAsyncTimeClocks::viaDetour(); }

inline int64_t OsGetAsyncClocksPerSecond() { return os::getAsyncClocksPerSecond::viaDetour(); }

#define sprintf (reinterpret_cast<int(__cdecl*)(char*, const char*, ...)>(0x0040C0F7))
#define strtod (reinterpret_cast<double(__cdecl*)(const char*, char**)>(0x0088E9C1))
#define strtoul (reinterpret_cast<unsigned long(__cdecl*)(const char*, char**, int)>(0x0040F640))
#define strcoll (reinterpret_cast<int(__cdecl*)(const char*, const char*)>(0x0088ED3C))
#define localeconv (reinterpret_cast<lconv*(__cdecl*)()>(0x00418030))
#define rand (reinterpret_cast<int(__cdecl*)()>(0x0088B867))
#define exit (reinterpret_cast<void(__cdecl*)(int)>(0x0040D150))

#undef isalpha
#undef isupper
#undef islower
#undef isdigit
#undef isxdigit
#undef isspace
#undef ispunct
#undef isalnum
#undef iscntrl
#undef tolower
#define isalpha luaswap_isalpha
#define isupper luaswap_isupper
#define islower luaswap_islower
#define isdigit luaswap_isdigit
#define isxdigit luaswap_isxdigit
#define isspace luaswap_isspace
#define ispunct luaswap_ispunct
#define isalnum luaswap_isalnum
#define iscntrl luaswap_iscntrl
#define tolower luaswap_tolower

// ctype functions (0x414003-0x41460B), inlined
inline bool luaswap_clocale() { return *reinterpret_cast<const int*>(0x00B3196C) == 0; }

inline const uint16_t* luaswap_pctype() { return *reinterpret_cast<const uint16_t* const*>(0x00AB71C8); }

#define LUASWAP_CTYPE(NAME, ADDRESS, MASK)                              \
    inline int luaswap_##NAME(int c) {                                  \
        if (luaswap_clocale()) { return luaswap_pctype()[c] & (MASK); } \
        return reinterpret_cast<int(__cdecl*)(int)>(ADDRESS)(c);        \
    }
LUASWAP_CTYPE(isalpha, 0x00414003, 0x103)
LUASWAP_CTYPE(isupper, 0x0041407D, 0x1)
LUASWAP_CTYPE(islower, 0x004140F5, 0x2)
LUASWAP_CTYPE(isdigit, 0x0041416D, 0x4)
LUASWAP_CTYPE(isxdigit, 0x004141EA, 0x80)
LUASWAP_CTYPE(isspace, 0x00414264, 0x8)
LUASWAP_CTYPE(ispunct, 0x004142DC, 0x10)
LUASWAP_CTYPE(isalnum, 0x00414359, 0x107)
LUASWAP_CTYPE(iscntrl, 0x00414452, 0x20)
#undef LUASWAP_CTYPE

inline int luaswap_tolower(int c) {
    if (luaswap_clocale()) { return static_cast<unsigned>(c - 'A') <= 'Z' - 'A' ? c + ('a' - 'A') : c; }
    return reinterpret_cast<int(__cdecl*)(int)>(0x004145E4)(c);
}
