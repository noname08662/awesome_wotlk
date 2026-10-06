#pragma once

#include <hookkit/accessor.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "NamePlateEntry.h"
#include "NamePlateSettings.h"

#include "include/Lib/Lua.h"
#include "include/ObjectManager/ObjectManager.h"
#include "include/UI/CLayoutFrame.h"
#include "include/World/CGWorldFrame.h"

class CGUnit_C;

namespace name_plates {
inline constexpr auto kEventCreated = "NAME_PLATE_CREATED";
inline constexpr auto kEventUnitAdded = "NAME_PLATE_UNIT_ADDED";
inline constexpr auto kEventUnitRemoved = "NAME_PLATE_UNIT_REMOVED";

enum class ClickLogic : uint8_t { eNone, eThruEnemy, eThruFriend };

enum class MouseInput : uint8_t { eIdle, eClickPending, eDragging };

class EntryManager {
public:
    std::vector<Entry*>& get() { return entries_; }

    [[nodiscard]]
    int getTotalSize() const {
        return std::ssize(by_id_);
    }

    [[nodiscard]]
    guid_t getTokenGuid(int index) const {
        if (index < 0 || index >= getTotalSize()) { return 0; }
        return by_id_.at(index).guid;
    }

    [[nodiscard]]
    int getTokenId(guid_t guid) const {
        if (guid == 0) { return -1; }
        for (int i = 0; i < getTotalSize(); ++i) {
            if (by_id_.at(i).guid == guid) { return i; }
        }
        return -1;
    }

    [[nodiscard]]
    const char* getToken(guid_t guid) const {
        return getToken(getTokenId(guid));
    }

    [[nodiscard]]
    const char* getToken(int index) const {
        if (index >= 0 && index < getTotalSize()) { return tokens_.at(index).c_str(); }
        return "none";  // this one is a valid unitId
    }

    Entry* getEntry(guid_t guid) { return getEntry(getTokenId(guid)); }

    Entry* getEntry(int index) {
        if (index < 0 || index >= getTotalSize()) { return nullptr; }
        Entry& e = by_id_.at(index);
        return (e.guid != 0) ? &e : nullptr;
    }

    CGNamePlateFrameExt* getPlate(guid_t guid) {
        const Entry* e = getEntry(guid);
        return e != nullptr ? e->ptr : nullptr;
    }

    // the plate frame passed as self (arg 1), if it's one of ours
    Entry* entryArg(LuaState* l);

    void init(int count);
    void clearAll();

    // per-frame layout pass, replaces CGWorldFrame::updateNamePlatePositions
    void update(CGWorldFrame* wf);

    // the player's reaction change affects every plate
    void onReactionChanged(CGUnit_C* unit);

    void refreshStacking() const {
        for (Entry* e : entries_) {
            e->updateStacking();
        }
    }

    void refreshClamping() const {
        for (Entry* e : entries_) {
            e->updateClamping();
        }
    }

    template <ClickLogic Mode>
    CGNamePlateFrameExt* pickPlate(Vec3f* pos) const {
        const guid_t target_guid = object_mgr::getTargetGuid();
        // a raised frozen plate keeps the cursor, the plates yielding to it slide through underneath;
        // unraised, it may sit under another plate and the click goes to what is visibly on top
        if (const RaiseFilter raise = mouseoverRaise(); raise.friendly || raise.enemy) {
            for (const Entry* e : entries_) {
                if (e->hasState(Entry::State::eIsFrozen) && raise.passes(e->hasState(Entry::State::eIsFriendly)) &&
                    target_guid != e->ptr->owner_guid_ && isUnderCursor(e, pos)) {
                    return e->ptr;
                }
            }
        }

        CGNamePlateFrameExt* prio = nullptr;
        for (const Entry* e : entries_) {
            if (target_guid == e->ptr->owner_guid_ || !isUnderCursor(e, pos)) { continue; }
            const bool is_friendly = e->hasState(Entry::State::eIsFriendly);

            if constexpr (Mode == ClickLogic::eThruEnemy) {
                if (is_friendly) { return e->ptr; }
            } else if constexpr (Mode == ClickLogic::eThruFriend) {
                if (!is_friendly) { return e->ptr; }
            } else {
                return e->ptr;
            }
            if (prio == nullptr) { prio = e->ptr; }
        }
        return prio;
    }

    // the one plate to freeze: the topmost of the focus and the target under the cursor. the engine never
    // focuses the target, so with the target on top the focus is a plate underneath it
    CGNamePlateFrameExt* hoverPlate(const CGNamePlateFrame* focus, const Vec3f* pos) const {
        const guid_t target_guid = object_mgr::getTargetGuid();
        for (const Entry* e : entries_) {  // still in the last draw order, topmost first
            if (e->ptr == focus) { return e->ptr; }
            if (target_guid != 0 && e->ptr->owner_guid_ == target_guid && isUnderCursor(e, pos)) { return e->ptr; }
        }
        return nullptr;
    }

    // plate under the cursor this frame, consumed by the next update
    void setHover(CGNamePlateFrame* plate) { hover_ = plate; }

private:
    // original logic, clamped search boundaries
    static bool isUnderCursor(const Entry* e, const Vec3f* pos) {
        if ((e->ptr->CLayoutFrame::flags_ & CLayoutFrame::FLAG_VALID_RECT) == 0) { return false; }
        const Settings& s = *kSettings;
        const Vec2f hit_box = e->hasState(Entry::State::eIsFriendly)
            ? Vec2f{s.hitbox_width_friend, s.hitbox_height_friend}
            : Vec2f{s.hitbox_width_enemy, s.hitbox_height_enemy};
        return e->ptr->isAtTargetPosPerc(pos, hit_box, static_cast<CLayoutFrame::HitboxAnchor>(s.hitbox_anchor));
    }

    enum class SortPass : uint8_t { eStack, eDraw };
    enum class SortMode : uint8_t { eDefault, eTarget, eFocus, eTargetFocus };

    struct alignas(32) PairState {
        uint64_t timestamp = 0;

        int hyst_steps = 0;
        float hyst_decay = 0.0f;
        float hysteresis = 1.0f;

        uint64_t proximate = 0;

        const Entry* e1 = nullptr;
        const Entry* e2 = nullptr;

        [[nodiscard]]
        bool isStale(uint64_t ms) const {
            return timestamp < ms;
        }

        [[nodiscard]]
        bool isApart(uint64_t ms) const {
            return proximate < ms;
        }

        void commit(uint64_t ms, float hyst) {
            if (hyst_decay > 0.0f && timestamp == 0) {
                hyst_steps++;
                hyst_decay = 1.0f;
            } else if (timestamp == 0) {
                hyst_decay = 1.0f;
            }
            timestamp = ms;
            hysteresis = hyst;
        }

        void cooldown(float delta) {
            if (e1 == nullptr || e2 == nullptr) {
                hyst_decay = 0.0f;
                hyst_steps = 0;
                return;
            }
            if (hyst_decay > 0.0f) {
                hyst_decay -= e1->getProximity(e2) * delta;
                if (hyst_decay <= 0.0f) {
                    hyst_decay = 0.0f;
                    hyst_steps = 0;
                }
            }
        }

        void seed(uint64_t ms, const Entry* first, const Entry* second) {
            proximate = ms;
            if (e1 == nullptr || e2 == nullptr) {
                e1 = first;
                e2 = second;
            }
        }

        void reset(bool full = false) {
            hysteresis = 1.0f;
            timestamp = 0;
            if (full) {
                proximate = 0;
                hyst_decay = 0.0f;
                hyst_steps = 0;
                e1 = nullptr;
                e2 = nullptr;
            }
        }
    };

    // strict lower triangle, rows keyed by the larger id: pairs among the first k ids pack into the
    // first k * (k - 1) / 2 slots. id1 != id2
    class PairsManager {
    public:
        void init(size_t count) { pairs_.assign(count * (count - 1) / 2, PairState{}); }

        void wipe() { std::ranges::fill(pairs_, PairState{}); }

        PairState* get(int id1, int id2) {
            const auto lo = static_cast<size_t>(std::min(id1, id2));
            const auto hi = static_cast<size_t>(std::max(id1, id2));
            return &pairs_[hi * (hi - 1) / 2 + lo];
        }

    private:
        std::vector<PairState> pairs_{};
    };

    PairState* getPair(const Entry* e1, const Entry* e2) {
        return pairs_mgr_.get(e1->ptr->getPlateId(), e2->ptr->getPlateId());
    }

    void commitPair(const Entry* e1, const Entry* e2, uint64_t ms, float delta, float by, float bx);
    void seedPair(Entry* e1, Entry* e2, uint64_t ms);
    void resolvePairs(Entry* e, uint64_t ms, float delta);

    void sort(SortPass pass);

    template <SortMode Mode, bool Out>
    void sortBy(guid_t target_guid = 0, CGNamePlateFrame* focus = nullptr, RaiseFilter raise = {}) {
        std::ranges::sort(entries_, [=](const Entry* a, const Entry* b) {
            if constexpr (Out) {
                // the mouseover raise, drawn on top so what the cursor sees is what it keeps
                const bool is_frozen_a =
                    a->hasState(Entry::State::eIsFrozen) && raise.passes(a->hasState(Entry::State::eIsFriendly));
                const bool is_frozen_b =
                    b->hasState(Entry::State::eIsFrozen) && raise.passes(b->hasState(Entry::State::eIsFriendly));
                if (is_frozen_a != is_frozen_b) { return is_frozen_a; }
            }
            if constexpr (Mode == SortMode::eTarget || Mode == SortMode::eTargetFocus) {
                const bool is_target_a = a->guid == target_guid;
                const bool is_target_b = b->guid == target_guid;
                if (is_target_a != is_target_b) { return is_target_a; }
            }
            if constexpr (Mode == SortMode::eFocus || Mode == SortMode::eTargetFocus) {
                const bool is_focus_a = a->ptr == focus;
                const bool is_focus_b = b->ptr == focus;
                if (is_focus_a != is_focus_b) { return is_focus_a; }
            }
            if constexpr (Out) {
                return a->ptr->depth_z_ < b->ptr->depth_z_;
            } else {
                const float pos_a = a->tar().y;
                const float pos_b = b->tar().y;
                if (std::abs(pos_a - pos_b) > kEps4f) { return pos_a < pos_b; }
                const int rank_a = a->getRankWeight();
                const int rank_b = b->getRankWeight();
                if (rank_a != rank_b) { return rank_a > rank_b; }
                return a->ptr < b->ptr;
            }
        });
    }

    void flushAdded();
    void flushRemoved();
    void appendAdded(int index);

    void clearPending() { std::fill_n(pending_.begin(), (getTotalSize() + 63) / 64, 0); }

    Entry* initEntry(CGNamePlateFrameExt* plate);
    void markRemoved(Entry* e, int id) const;
    void track(CGNamePlateFrameExt* plate);
    MouseInput mouseInput(uint64_t ms);
    void stack(uint64_t ms, float scene_time, float pull_scale);

    static void onPlateCreated(CGNamePlateFrameExt* plate);

    PairsManager pairs_mgr_;

    std::vector<Entry> by_id_;
    std::vector<Entry*> entries_;
    std::vector<std::string> tokens_;

    std::array<uint64_t, kMaxPlates / 64> pending_{};

    CGNamePlateFrame* hover_ = nullptr;
    CGNamePlateFrame* last_hover_ = nullptr;
    uint64_t press_ms_ = 0;
};

inline constexpr utils::Accessor<EntryManager, struct PlatesTag> kPlates;
}  // namespace name_plates
