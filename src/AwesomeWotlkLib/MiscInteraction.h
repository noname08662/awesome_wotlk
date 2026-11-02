#pragma once

#include <corecrt_math_defines.h>
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <type_traits>
#include <utility>

#include "Extensions.h"

#include "include/Game/CGGameUI.h"
#include "include/Lib/Storm.h"
#include "include/M2/CM2Model.h"
#include "include/Math/Primitives.h"
#include "include/ObjectManager/CGGameObject_C.h"
#include "include/ObjectManager/CGPlayer_C.h"
#include "include/ObjectManager/DescriptorsEnums.h"
#include "include/ObjectManager/ObjectManager.h"
#include "include/ObjectManager/ObjectManagerEnums.h"

namespace misc_interaction {
inline constexpr DWORD kCandidateScanIntervalMs = 100;

inline constexpr auto kInteractTypemask = static_cast<TypeMask>(eTypemaskGameobject | eTypemaskUnit);

struct CycleStamp {
    guid_t guid;
    uint32_t stamp;

    static uint32_t find(const TSGrowableArray<CycleStamp>& stamps, guid_t guid) {
        const CycleStamp* it = std::ranges::find(stamps, guid, &CycleStamp::guid);
        return it != stamps.end() ? it->stamp : 0;
    }

    static void assign(TSGrowableArray<CycleStamp>& stamps, guid_t guid, uint32_t stamp) {
        if (CycleStamp* it = std::ranges::find(stamps, guid, &CycleStamp::guid); it != stamps.end()) {
            it->stamp = stamp;
        } else {
            stamps.push_back({.guid = guid, .stamp = stamp});
        }
    }

    static void prune(TSGrowableArray<CycleStamp>& stamps) {
        auto gone = std::ranges::remove_if(stamps, [](const CycleStamp& entry) {
            return object_mgr::get<CGObject_C>(entry.guid, kInteractTypemask) == nullptr;
        });
        stamps.setCount(static_cast<uint32_t>(gone.begin() - stamps.begin()));
    }
};

struct InteractState {
    guid_t request = 0;      // one-shot interaction queued from Lua
    guid_t candidate = 0;    // valid interact target the next press goes to
    guid_t highlighted = 0;  // candidate currently showing the interact highlight (in range, cvar on)
    DWORD last_scan_tick = 0;
    uint32_t cycle_counter = 0;
    // targets already interacted with; dropped once the object is gone
    TSGrowableArray<CycleStamp> cycle_stamps{.chunk = 8};

    void reset() {
        request = 0;
        candidate = 0;
        highlighted = 0;
        last_scan_tick = 0;
        cycle_counter = 0;
        cycle_stamps.clear();
    }

    void queue(guid_t guid) {
        if (guid != 0) { request = guid; }
    }

    void queueCandidate() { queue(candidate); }

    void stampInteracted(guid_t guid) {
        CycleStamp::assign(cycle_stamps, guid, ++cycle_counter);
        last_scan_tick = GetTickCount() - kCandidateScanIntervalMs;  // move candidate + highlight next frame
    }

    void processRequest() {
        if (request == 0) { return; }
        if (auto* object = object_mgr::get<CGObject_C>(request, kInteractTypemask)) {
            stampInteracted(request);
            object->onRightClick();
        }
        request = 0;
    }

    void updateCandidate();
    void setHighlighted(guid_t guid);

    void onHighlightHidden(CGObject_C* object, CGObject_C::HighlightType highlight_type) const {
        if (highlight_type != CGObject_C::eHighlightTypeInteract && highlighted != 0 &&
            object->getGuid() == highlighted) {
            dimHighlight(object);
        }
    }

    static bool isLootTarget(CGObject_C* object);
    static bool isValidTarget(CGObject_C* object, CGUnit_C* player, float distance_sq, bool is_loot);

    static void dimHighlight(CGObject_C* object) {
        if ((object->highlight_mask_ & CGObject_C::eHighlightMaskNative) != 0) { return; }
        if (CM2Model* model = object->getObjectModel()) { model->emissive_color_ *= 0.8f; }
    }
};

static_assert(std::is_trivially_destructible_v<InteractState>);
inline constinit InteractState interact{};

inline void InteractState::updateCandidate() {
    if (!game_ui::isInWorld()) { return; }

    DWORD now = GetTickCount();
    if (now - last_scan_tick < kCandidateScanIntervalMs) { return; }
    last_scan_tick = now;

    CycleStamp::prune(cycle_stamps);

    struct CycleEntry {
        guid_t guid = 0;
        uint32_t stamp = UINT32_MAX;
        float distance_sq = 400.0f;  // 20.0f squared

        [[nodiscard]]
        auto key() const { return std::pair{stamp, distance_sq}; }
    };

    CycleEntry best_loot;
    CycleEntry nearest_other;

    auto* player = object_mgr::get<CGPlayer_C>(object_mgr::getPlayerGuid(), eTypemaskPlayer);
    if (player == nullptr) { return; }

    auto angle_degrees = static_cast<float>(extensions::console::kCvarRegistry->ref<"interactionAngle", int>()) * 0.5f;
    bool look_in_angle = extensions::console::kCvarRegistry->ref<"interactionMode", int>() == 1;

    float facing = player->getFacing();
    Vec3f pos_player{};
    player->getPosition(pos_player);

    auto try_set_candidate = [&](guid_t guid) {
        auto* object = object_mgr::get<CGObject_C>(guid, kInteractTypemask);
        if (object == nullptr) { return; }

        auto distance_sq = static_cast<float>(object->getDistanceToPosSq(&pos_player));
        if (distance_sq == 0.0f || distance_sq > 400.0f) { return; }

        bool is_loot = isLootTarget(object);
        CycleEntry entry{.guid = guid, .stamp = CycleStamp::find(cycle_stamps, guid), .distance_sq = distance_sq};
        if (is_loot ? entry.key() > best_loot.key() : distance_sq > nearest_other.distance_sq) { return; }

        if (!isValidTarget(object, player, distance_sq, is_loot)) { return; }

        if (look_in_angle) {
            Vec3f pos_object{};
            object->getPosition(pos_object);
            float dx = pos_object.x - pos_player.x;
            float dy = pos_object.y - pos_player.y;

            float length_sq = dx * dx + dy * dy;
            if (length_sq == 0.0f) { return; }

            float length = std::sqrtf(length_sq);
            dx /= length;
            dy /= length;

            if (dx * std::cosf(facing) + dy * std::sinf(facing) <
                std::cosf(angle_degrees * static_cast<float>(M_PI / 180.0))) {
                return;
            }
        }

        (is_loot ? best_loot : nearest_other) = entry;
    };

    object_mgr::enumObjects([&](guid_t guid) {
        if (guid != player->getGuid()) { try_set_candidate(guid); }
        return true;
    });

    candidate = nearest_other.key() < best_loot.key() ? nearest_other.guid : best_loot.guid;
    setHighlighted((extensions::console::kCvarRegistry->ref<"interactionHighlight", int>() != 0) ? candidate : 0);
}

inline void InteractState::setHighlighted(guid_t guid) {
    if (highlighted == guid) { return; }
    if (highlighted != 0) {
        if (auto* old_obj = object_mgr::get<CGObject_C>(highlighted, kInteractTypemask)) {
            old_obj->hideHighlightType(CGObject_C::eHighlightTypeInteract);
        }
    }
    highlighted = guid;
    if (highlighted != 0) {
        if (auto* new_obj = object_mgr::get<CGObject_C>(highlighted, kInteractTypemask)) {
            new_obj->showHighlightType(CGObject_C::eHighlightTypeInteract);
            dimHighlight(new_obj);
        }
    }
}

inline bool InteractState::isLootTarget(CGObject_C* object) {
    if (object->type_id_ == eTypeidUnit) {
        auto* unit = object->as<CGUnit_C>();
        return unit->isLootable() || unit->canSkin();
    }
    if (object->type_id_ == eTypeidGameobject) {
        auto* go = object->as<CGGameObject_C>();
        return go->descriptors_->subtype == eGameobjectTypeChest && !go->isGatheringNode();
    }
    return false;
}

inline bool InteractState::isValidTarget(CGObject_C* object, CGUnit_C* player, float distance_sq, bool is_loot) {
    if (object->type_id_ == eTypeidUnit) {
        auto* unit = object->as<CGUnit_C>();
        uint32_t npc_flags = unit->getValue<uint32_t>(eUnitNpcFlags);
        bool can_assist = player->canAssist(unit, true);

        bool flags_ok = is_loot || (can_assist && npc_flags != 0);
        return flags_ok && distance_sq <= unit->getInteractDistanceSq();
    }
    if (object->type_id_ == eTypeidGameobject) { return object->as<CGGameObject_C>()->canInteract(); }
    return false;
}
}  // namespace misc_interaction
