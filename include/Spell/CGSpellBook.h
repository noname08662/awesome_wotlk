#pragma once

#include <hookkit/hookkit.h>

#include <cstdint>

class CGUnit_C;

class CGSpellBook {
public:
    HOOKKIT_HOOK_HANDLE(getSkinningSpell, 0x0053BCE0, hookkit::Conv::eCdecl, uint32_t, CGUnit_C*);
};
