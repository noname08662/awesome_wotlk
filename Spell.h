#pragma once
#include "Enums.h"
#include <array>
#include <optional>

namespace Spell {
    constexpr std::array<std::pair<ShapeshiftForm, uint32_t>, 9> ShapeshiftFormSpells = {
        {
            { ShapeshiftForm::FORM_CAT, 768 },
            { ShapeshiftForm::FORM_TRAVEL, 783 },
            { ShapeshiftForm::FORM_AQUA, 1066 },
            { ShapeshiftForm::FORM_BEAR, 5487 },
            { ShapeshiftForm::FORM_DIREBEAR, 9634 },
            { ShapeshiftForm::FORM_TREE, 33891 },
            { ShapeshiftForm::FORM_BATTLESTANCE, 2457 },
            { ShapeshiftForm::FORM_DEFENSIVESTANCE, 71 },
            { ShapeshiftForm::FORM_BERSERKERSTANCE, 2458 },
        }
    };

    inline std::optional<uint32_t> GetSpellId(ShapeshiftForm form) {
        for (const auto& [f, spell] : ShapeshiftFormSpells) {
            if (f == form) return spell;
        }
        return std::nullopt;
    }

    inline std::optional<ShapeshiftForm> GetFormFromSpell(uint32_t spellId) {
        for (const auto& [f, spell] : ShapeshiftFormSpells) {
            if (spell == spellId) return f;
        }
        return std::nullopt;
    }

    inline bool IsForm(uint32_t spellId) {
        return GetFormFromSpell(spellId).has_value();
    }
    void initialize();
}