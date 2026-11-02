#pragma once

#include <hookkit/hook.h>

#include "BaseTypes.h"

#include "Widget/CSimpleTop.h"

class CTexture;

namespace game_ui {
constexpr uintptr_t kTabTargetRangeCapSqAddr = 0x009FE7F8;
constexpr uintptr_t kPortraitDepthCallbackAddr = 0x005EEB70;

inline auto& portrait_use_render_target = *reinterpret_cast<int*>(0x00C5CDFC);
inline auto& portrait_depth_texture = *reinterpret_cast<CTexture**>(0x00C5CDF8);

inline static auto& nameplate_max_distance_sq = *reinterpret_cast<float*>(0x00ADAA7C);
inline static auto& nameplate_show_enemies = *reinterpret_cast<CVar**>(0x00BD09B8);
inline static auto& nameplate_show_friends = *reinterpret_cast<CVar**>(0x00BD09C8);

enum LogType : uint32_t {
    eLogChat = 0,
    eLogCombat = 1,
};

inline auto& log_file_names = *reinterpret_cast<const char* (*)[2]>(0x00AC7A40);
inline const auto kDefaultChatLogFile = reinterpret_cast<const char*>(0x009FA4E4);    // Logs\WoWChatLog.txt
inline const auto kDefaultCombatLogFile = reinterpret_cast<const char*>(0x009FA4CC);  // Logs\WoWCombatLog.txt

inline auto& zone_text = *reinterpret_cast<const char**>(0x00BD0788);       // GetZoneText
inline auto& sub_zone_text = *reinterpret_cast<const char**>(0x00BD0784);   // GetSubZoneText
inline auto& real_zone_text = *reinterpret_cast<const char**>(0x00BD0780);  // GetRealZoneText
inline auto& zone_area_id = *reinterpret_cast<int32_t*>(0x00BD080C);        // AreaTable id of the zone
inline auto& sub_zone_area_id = *reinterpret_cast<int32_t*>(0x00BD0810);  // AreaTable id of the subzone, 0 outside one
inline auto& current_map_id = *reinterpret_cast<int32_t*>(0x00BD088C);    // Map.dbc id

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
