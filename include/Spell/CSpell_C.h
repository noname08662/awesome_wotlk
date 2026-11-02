#pragma once

#include <cstdint>

#include "BaseTypes.h"

#include "Item/CGItem_C.h"
#include "ObjectManager/CGUnit_C.h"

class CSpell_C {
public:
    struct CPendingSpellCast {
        unk_t _unk_00[4];
        guid_t unit_guid;  // spell range and radius are measured from this unit
        unk_t _unk_18[2];
        uint32_t spell_id;
        unk_t _unk_24[73];
    };

    static_assert(sizeof(CPendingSpellCast) == 0x148);

    inline static auto& pending_spell_cast = *reinterpret_cast<CPendingSpellCast**>(0x00D3F4E4);

    HOOKKIT_HOOK_HANDLE(isTargetingAoE, 0x007FD620, hookkit::Conv::eCdecl, bool);
    HOOKKIT_HOOK_HANDLE(canTargetTerrain, 0x007FD750, hookkit::Conv::eCdecl, bool);
    // unit, spell id, min out, max out, target
    HOOKKIT_HOOK_HANDLE(
        getSpellRange, 0x00802C30, hookkit::Conv::eCdecl, void, CGUnit_C*, uint32_t, float*, float*, CGUnit_C*);
    HOOKKIT_HOOK_HANDLE(castSpell, 0x0080CCE0, hookkit::Conv::eCdecl, int, CGUnit_C*, uint32_t, CGItem_C*, guid_t,
        CPendingSpellCast*, bool);
    HOOKKIT_HOOK_HANDLE(cancelPendingAoeTargeting, 0x007FCC30, hookkit::Conv::eCdecl, int);
};
