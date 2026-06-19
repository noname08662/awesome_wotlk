#pragma once
#include "Enums.h"
#include <array>
#include <optional>

namespace Spell {
constexpr std::array<std::pair<ShapeshiftForm, uint32_t>, 9> ShapeshiftFormSpells = {{{FORM_CAT, 768}, {FORM_TRAVEL, 783}, {FORM_AQUA, 1066}, {FORM_BEAR, 5487}, {FORM_DIREBEAR, 9634}, {FORM_TREE, 33891}, {FORM_BATTLESTANCE, 2457}, {FORM_DEFENSIVESTANCE, 71}, {FORM_BERSERKERSTANCE, 2458},}};

inline std::optional<uint32_t> GetSpellId(ShapeshiftForm form) {
	for (const auto& [f, spell] : ShapeshiftFormSpells) { if (f == form) return spell; }
	return std::nullopt;
}

inline std::optional<ShapeshiftForm> GetFormFromSpell(uint32_t spellId) {
	for (const auto& [f, spell] : ShapeshiftFormSpells) { if (spell == spellId) return f; }
	return std::nullopt;
}

inline bool IsForm(uint32_t spellId) { return GetFormFromSpell(spellId).has_value(); }
void initialize();
}
