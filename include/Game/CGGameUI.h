#pragma once

#include <hookkit/hook.h>

#include "BaseTypes.h"

#include "Widget/CSimpleTop.h"

class CTexture;

namespace game_ui {
constexpr uintptr_t kTabTargetRangeCapSqAddr = 0x009FE7F8;

inline auto& portrait_use_render_target = *reinterpret_cast<int*>(0x00C5CDFC);
inline auto& portrait_depth_texture = *reinterpret_cast<CTexture**>(0x00C5CDF8);

HOOKKIT_HOOK_HANDLE(target, 0x00524BF0, hookkit::Conv::eCdecl, void, guid_t);
HOOKKIT_HOOK_HANDLE(clearTarget, 0x005241B0, hookkit::Conv::eCdecl, void, guid_t, int);
HOOKKIT_HOOK_HANDLE(updateUnitHighlights, 0x00513CF0, hookkit::Conv::eCdecl, void);
HOOKKIT_HOOK_HANDLE(enterWorld, 0x00528010, hookkit::Conv::eFastcall, void);
HOOKKIT_HOOK_HANDLE(leaveWorld, 0x00528C30, hookkit::Conv::eFastcall, void);

inline bool inCombatLockdown() {
    auto* top = CSimpleTop::get();
    if (top == nullptr) { return false; }
    return top->protected_actions_allowed_ == 0;
}

inline bool isInWorld() { return *reinterpret_cast<char*>(0x00BD0792) != 0; }
}  // namespace game_ui
