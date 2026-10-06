#pragma once

#include <cstdint>

#include "BaseTypes.h"

#include "Item/CGItem_C.h"
#include "ObjectManager/CGUnit_C.h"

class CSpell_C {
public:
    struct CPendingSpellCast {
        unk_t _unk_00[8];
        uint32_t spell_id;
        unk_t _unk_24[73];
    };

    inline static auto& pending_spell_cast = *reinterpret_cast<CPendingSpellCast**>(0x00D3F4E4);

    HOOKKIT_HOOK_HANDLE(isTargetingAoE, 0x007FD620, hookkit::Conv::eCdecl, bool);
    HOOKKIT_HOOK_HANDLE(castSpell, 0x0080CCE0, hookkit::Conv::eCdecl, int, CGUnit_C*, uint32_t, CGItem_C*, guid_t,
        CPendingSpellCast*, bool);
    HOOKKIT_HOOK_HANDLE(cancelPendingAoeTargeting, 0x007FCC30, hookkit::Conv::eCdecl, int);
};
