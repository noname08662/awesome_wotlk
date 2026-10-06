#pragma once

#include <hookkit/accessor.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "Extensions.h"

#include "include/Game/CGGameUI.h"
#include "include/ObjectManager/ObjectManagerEnums.h"
#include "include/World/CGWorldFrame.h"

namespace name_plates {
inline constexpr uint16_t kMaxPlates = 768;
static_assert(kMaxPlates % 64 == 0, "pending bitset words are 64 plates wide");

enum class StackingMode : int { eDisabled = 0, eAll = 1, eEnemy = 2, eFriendly = 3 };

enum class ClampMode : int { eDisabled = 0, eAll = 1, eBoss = 2, eAllEdges = 3, eBossEdges = 4 };

enum class ClampFilter : int { eNone = 0, eTarget = 1 << 0, eCombat = 1 << 1, eTargetCombat = eTarget | eCombat };

enum class OcclusionMode : int { eAlways = 0, eNotInCombat = 1 };

enum class FreezeMode : int { eDisabled = 0, eAlways = 1, eCombat = 2 };

enum class MouseMode : uint32_t {
    eDisabled = 0,
    eClickThruEnemy = 1 << 0,
    eClickThruFriend = 1 << 1,
    eOverAlways = 1 << 2,
    eOverCombat = 1 << 3,
};

constexpr MouseMode operator|(MouseMode a, MouseMode b) {
    return static_cast<MouseMode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

constexpr bool hasFlag(MouseMode mode, MouseMode flag) {
    return (static_cast<uint32_t>(mode) & static_cast<uint32_t>(flag)) != 0;
}

inline constexpr std::array kMouseModeMap = {
    MouseMode::eDisabled,
    MouseMode::eClickThruEnemy,
    MouseMode::eClickThruEnemy | MouseMode::eOverAlways,
    MouseMode::eClickThruEnemy | MouseMode::eOverCombat,
    MouseMode::eClickThruFriend,
    MouseMode::eClickThruFriend | MouseMode::eOverAlways,
    MouseMode::eClickThruFriend | MouseMode::eOverCombat,
    MouseMode::eOverAlways,
    MouseMode::eOverCombat,
};

struct Settings {
    const int& stacking;
    const float& band_x;
    const float& band_y;
    const int& hitbox_anchor;
    const float& hitbox_width_enemy;
    const float& hitbox_height_enemy;
    const float& hitbox_width_friend;
    const float& hitbox_height_friend;
    const float& placement;
    const int& mouse_mode;
    const float& raise_speed;
    const float& lower_speed;
    const float& pull_speed;
    const float& raise_distance;
    const float& pull_distance;
    const float& occlusion_alpha;
    const int& occlusion_mode;
    const float& non_target_alpha;
    const float& alpha_speed;
    const float& inertia;
    const float& hysteresis_decay;
    const int& clamp_mode;
    const float& clamp_v_offset;
    const float& clamp_h_offset;
    const int& clamp_filter;
    const int& freeze_mode;
    const float& freeze_grace;
    const float& freeze_time;

    [[nodiscard]]
    StackingMode stackingMode() const {
        return static_cast<StackingMode>(std::abs(stacking));
    }

    [[nodiscard]]
    ClampMode clampMode() const {
        return static_cast<ClampMode>(clamp_mode);
    }

    [[nodiscard]]
    bool clampAllEdges() const {
        return clampMode() == ClampMode::eAllEdges || clampMode() == ClampMode::eBossEdges;
    }

    template <typename RankFn>
    [[nodiscard]]
    bool clampsRank(const RankFn& rank) const {
        switch (clampMode()) {
            case ClampMode::eAll:
            case ClampMode::eAllEdges:
                return true;
            case ClampMode::eBoss:
            case ClampMode::eBossEdges:
                return rank() == eRankWorldboss;
            case ClampMode::eDisabled:
            default:
                return false;
        }
    }

    [[nodiscard]]
    bool clampFilterPasses(guid_t guid, guid_t target, bool in_combat) const {
        const auto filter = static_cast<uint32_t>(clamp_filter);
        const bool target_ok =
            (filter & static_cast<uint32_t>(ClampFilter::eTarget)) == 0 || (guid != 0 && guid == target);
        const bool combat_ok = (filter & static_cast<uint32_t>(ClampFilter::eCombat)) == 0 || in_combat;
        return target_ok && combat_ok;
    }

    [[nodiscard]]
    bool freezeActive() const {
        const auto mode = static_cast<FreezeMode>(freeze_mode);
        return mode == FreezeMode::eAlways || (mode == FreezeMode::eCombat && game_ui::inCombatLockdown());
    }

    // the cvars must be registered before the first access
    static Settings make() {
        const auto& cvars = *extensions::console::kCvarRegistry;
        return Settings{
            .stacking = cvars.ref<int>("nameplateStacking"),
            .band_x = cvars.ref<float>("nameplateBandX"),
            .band_y = cvars.ref<float>("nameplateBandY"),
            .hitbox_anchor = cvars.ref<int>("nameplateHitboxAnchor"),
            .hitbox_width_enemy = cvars.ref<float>("nameplateHitboxWidthE"),
            .hitbox_height_enemy = cvars.ref<float>("nameplateHitboxHeightE"),
            .hitbox_width_friend = cvars.ref<float>("nameplateHitboxWidthF"),
            .hitbox_height_friend = cvars.ref<float>("nameplateHitboxHeightF"),
            .placement = cvars.ref<float>("nameplatePlacement"),
            .mouse_mode = cvars.ref<int>("nameplateMouseMode"),
            .raise_speed = cvars.ref<float>("nameplateRaiseSpeed"),
            .lower_speed = cvars.ref<float>("nameplateLowerSpeed"),
            .pull_speed = cvars.ref<float>("nameplatePullSpeed"),
            .raise_distance = cvars.ref<float>("nameplateRaiseDistance"),
            .pull_distance = cvars.ref<float>("nameplatePullDistance"),
            .occlusion_alpha = cvars.ref<float>("nameplateOcclusionAlpha"),
            .occlusion_mode = cvars.ref<int>("nameplateOcclusionMode"),
            .non_target_alpha = cvars.ref<float>("nameplateNonTargetAlpha"),
            .alpha_speed = cvars.ref<float>("nameplateAlphaSpeed"),
            .inertia = cvars.ref<float>("nameplateInertia"),
            .hysteresis_decay = cvars.ref<float>("nameplateHysteresisDecay"),
            .clamp_mode = cvars.ref<int>("nameplateClampMode"),
            .clamp_v_offset = cvars.ref<float>("nameplateClampModeVOffset"),
            .clamp_h_offset = cvars.ref<float>("nameplateClampModeHOffset"),
            .clamp_filter = cvars.ref<int>("nameplateClampModeFilter"),
            .freeze_mode = cvars.ref<int>("nameplateMouseFreeze"),
            .freeze_grace = cvars.ref<float>("nameplateMouseFreezeGrace"),
            .freeze_time = cvars.ref<float>("nameplateMouseFreezeTime"),
        };
    }
};

inline constexpr utils::Accessor<const Settings, struct SettingsTag, Settings::make> kSettings;

inline constexpr float kTunedFps = 60.0f;

inline float perFrame(float k, float delta) { return 1.0f - std::pow(1.0f - std::min(k, 0.99f), delta * kTunedFps); }

// capped so frames below 50 fps slow the animations down instead of jumping
inline float sceneDelta(const CGWorldFrame* wf) { return std::min(0.02f, wf->scene_time_); }

struct Derived {
    MouseMode mouse_mode = MouseMode::eDisabled;
    uint8_t non_target_alpha = 255;
    bool occlusion_on = false;
    bool occlude_up = false;  // a negative alpha caps the plate alpha instead of scaling it
    uint8_t occlusion_cap = 255;
    float occlusion_scale = 1.0f;
    float raise_gain = 1.0f;
    float lower_gain = 1.0f;
    float pull_gain = 1.0f;
    float lower_settle = 0.5f;  // freshState's decay rate, before perFrame

    static Derived make(const Settings& s) {
        const auto gain = [](float speed) { return std::pow(std::clamp(speed / kTunedFps, 0.0f, 1.0f), 1.5f); };
        const float occlusion = std::abs(s.occlusion_alpha);
        return Derived{
            .mouse_mode = kMouseModeMap.at(static_cast<size_t>(s.mouse_mode)),
            .non_target_alpha = static_cast<uint8_t>(255 * s.non_target_alpha),
            .occlusion_on = occlusion < 1.0f,
            .occlude_up = s.occlusion_alpha < 0.0f,
            .occlusion_cap = static_cast<uint8_t>(255 * occlusion),
            .occlusion_scale = occlusion,
            .raise_gain = gain(s.raise_speed),
            .lower_gain = gain(s.lower_speed),
            .pull_gain = gain(s.pull_speed),
            .lower_settle = std::clamp(s.lower_speed / kTunedFps, 0.0f, 0.5f),
        };
    }
};

inline constinit Derived derived_settings{};

// forces the engine to recompute plate positions next frame
inline void markDirty() {
    if (CGWorldFrame* wf = CGWorldFrame::get()) { wf->render_dirty_flags_ |= 1; }
}

inline bool focusHasPriority() {
    const MouseMode mode = derived_settings.mouse_mode;
    return hasFlag(mode, MouseMode::eOverAlways) ||
        (hasFlag(mode, MouseMode::eOverCombat) && game_ui::inCombatLockdown());
}

struct RaiseFilter {
    bool friendly = false;
    bool enemy = false;

    [[nodiscard]]
    bool passes(bool is_friendly) const {
        return is_friendly ? friendly : enemy;
    }
};

inline RaiseFilter mouseoverRaise() {
    if (!focusHasPriority()) { return {}; }
    const MouseMode mode = derived_settings.mouse_mode;
    return {
        .friendly = !hasFlag(mode, MouseMode::eClickThruFriend), .enemy = !hasFlag(mode, MouseMode::eClickThruEnemy)};
}
}  // namespace name_plates
