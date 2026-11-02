#include "NamePlateManager.h"

#include <bit>
#include <format>
#include <utility>

#include "Extensions.h"

#include "include/Game/CGInputControl.h"
#include "include/ObjectManager/CGUnit_C.h"
#include "include/ObjectManager/ObjectManager.h"
#include "include/ObjectManager/ObjectManagerEnums.h"
#include "include/System/System.h"

namespace name_plates {
namespace {
using extensions::console::kEventRegistry;

int luaSetStackingEnabled(LuaState* l) {
    if (Entry* e = kPlates->entryArg(l)) {
        e->setState(Entry::State::eIsTransient, !lua::toBoolean(l, 2));
        e->updateStacking();
    }
    return 0;
}

int luaGetStackingEnabled(LuaState* l) {
    if (const Entry* e = kPlates->entryArg(l)) {
        lua::pushBoolean(l, e->hasState(Entry::State::eShouldStack));
        return 1;
    }
    return 0;
}

int luaSetOcclusionEnabled(LuaState* l) {
    if (const Entry* e = kPlates->entryArg(l)) { e->ptr->setOpaque(!lua::toBoolean(l, 2)); }
    return 0;
}

int luaGetOcclusionEnabled(LuaState* l) {
    if (const Entry* e = kPlates->entryArg(l)) {
        lua::pushBoolean(l, !e->ptr->isOpaque());
        return 1;
    }
    return 0;
}

constexpr std::array<lua::LuaLReg, 4> kMethods = {{
    {.name = "SetStackingEnabled", .func = luaSetStackingEnabled},
    {.name = "GetStackingEnabled", .func = luaGetStackingEnabled},
    {.name = "SetOcclusionEnabled", .func = luaSetOcclusionEnabled},
    {.name = "GetOcclusionEnabled", .func = luaGetOcclusionEnabled},
}};
}  // namespace

Entry* EntryManager::entryArg(LuaState* l) {
    if (!lua::isTable(l, 1)) { return nullptr; }

    CSimpleFrame* frame = lua::toFrameSilent(l, 1);
    if (frame == nullptr) { return nullptr; }

    auto* plate = frame->as<CGNamePlateFrameExt>();
    const int index = plate->getPlateId();
    if (index >= 0 && index < getTotalSize() && by_id_.at(index).ptr == plate) { return &by_id_.at(index); }
    return nullptr;
}

void EntryManager::init(int count) {
    tokens_.reserve(count);
    for (int i = 1; i <= count; ++i) {
        tokens_.push_back(std::format("nameplate{}", i));
    }
    by_id_.reserve(count);
    pairs_mgr_.init(count);
    entries_.reserve(count);
}

void EntryManager::clearAll() {
    pairs_mgr_.wipe();
    by_id_.clear();
    entries_.clear();
    pending_.fill(0);
    hover_ = 0;
}

void EntryManager::update(CGWorldFrame* wf) {
    // every plate the engine has built, hidden and free-listed ones included, nothing else
    CGNamePlateFrame::CDA.forEachBlock([this](CGNamePlateFrame* block) {
        if (CGNamePlateFrame::isConstructed(block)) { track(CGNamePlateFrameExt::of(block)); }
    });

    // bulk flush now so callbacks receive a complete gapless snapshot of the previous frame
    flushRemoved();
    const uint64_t ms = os::getAsyncTimeMs{}();
    const CGInputControl* input = CGInputControl::get{}();
    const bool dragging = input != nullptr && input->isDragging(static_cast<uint32_t>(ms));
    const uintptr_t hover_bits = dragging ? 0 : hover_;  // the plate pressed on stays pinned through a click only
    hover_ = 0;
    const auto* hover = reinterpret_cast<const CGNamePlateFrameExt*>(hover_bits & ~kHoverPinned);
    const bool pinned = (hover_bits & kHoverPinned) != 0;

    if (entries_.empty()) {
        clearPending();
        CLayoutFrame::resizePending{}();
        return;
    }

    const Settings& s = *kSettings;
    uint32_t level = entries_.size() * 10;
    const float scene_time = sceneDelta(wf);
    const guid_t target_guid = object_mgr::getTargetGuid();
    const bool in_combat = game_ui::inCombatLockdown();
    const bool release_now = hover != nullptr || dragging;

    // the freeze mode is the focus hook's detour, inactive it leaves the hover unset
    for (Entry* e : entries_) {
        e->updateFreeze(pinned && e->ptr == hover, release_now, ms, s.freeze_grace);
    }

    sort(SortPass::eStack);  // no target/focus
    const Derived& d = derived_settings;
    const float settle = perFrame(d.lower_settle, scene_time);
    // accum_x sums pulls across frames, rescaled so its steady state matches the tuned rate at any fps
    const float pull_scale = settle * (1.0f - d.lower_settle) / (d.lower_settle * (1.0f - settle));
    for (Entry* e : entries_) {
        e->freshState(settle, s.pull_distance);
    }

    stack(ms, scene_time, pull_scale);

    sort(SortPass::eDraw);
    for (Entry* e : entries_) {
        resolvePairs(e, ms, scene_time);
        const bool raising = (e->commit_target.y - e->stack_offset.y) > 0.0f;
        e->updVis({
            .speed = {s.pull_speed, raising ? s.raise_speed : s.lower_speed},
            .gain = {d.pull_gain, raising ? d.raise_gain : d.lower_gain},
            .inertia = s.inertia,
            .delta = scene_time,
            .max_offset = {s.pull_distance, s.raise_distance},
            .ceil = {s.clamp_h_offset, s.clamp_v_offset},
            .all_edges = s.clampAllEdges(),
            // owner_guid_, e->guid is only assigned in flushAdded after this pass
            .clamp = e->hasState(Entry::State::eShouldClamp) &&
                s.clampFilterPasses(e->ptr->owner_guid_, target_guid, in_combat),
        });
        e->setState(Entry::State::eIsFresh, false);
        const Vec2f vis = e->place(s.freeze_time, scene_time) / e->ptr->CLayoutFrame::scale_;
        e->ptr->setPoint(CLayoutFrame::eAnchorTop, wf, CLayoutFrame::eAnchorBottomleft, vis.x, vis.y, true);
        e->ptr->setFrameDepth(e->ptr->depth_z_ - wf->depth_, 1);
        e->ptr->setFrameLevel(level, 1);
        level -= 10;  // addons buffer
    }
    // clearing this stops further calls until any plate's raw ndc changes
    wf->render_dirty_flags_ |= 1;

    CLayoutFrame::resizePending{}();
    flushAdded();  // all set, fire callbacks
    clearPending();
}

void EntryManager::onReactionChanged(CGUnit_C* unit) {
    if (unit->getGuid() == object_mgr::getPlayerGuid()) {
        for (Entry* e : entries_) {
            e->updateReaction();
        }
    } else if (CGNamePlateFrame* plate = unit->nameplate_) {
        if (Entry* e = getEntry(CGNamePlateFrameExt::of(plate)->getPlateId())) { e->updateReaction(); }
    }
}

void EntryManager::commitPair(const Entry* e1, const Entry* e2, uint64_t ms, float delta, float by, float bx) {
    auto* ps = getPair(e1, e2);
    if ((e1->getTopNdc() + e1->target_offset.y + e1->getAvgHFor(e2, by)) > (e2->getBotNdc() + e2->target_offset.y) &&
        e1->getReqDxFor(e2) < e1->getAvgWFor(e2, bx)) {
        ps->commit(ms, 1.25f + std::min(static_cast<float>(ps->hyst_steps) * 0.15f, 0.75f));  // still overlapping
    } else {
        // bboxes no longer overlap, decay scaled by separation
        ps->commit(
            ms, std::max(1.0f, ps->hysteresis - (e1->getProximity(e2) * 0.05f) * delta * kSettings->hysteresis_decay));
    }
}

void EntryManager::seedPair(Entry* e1, Entry* e2, uint64_t ms) {
    const int id1 = e1->ptr->getPlateId();
    const int id2 = e2->ptr->getPlateId();
    pairs_mgr_.get(id1, id2)->seed(ms, e1, e2);
    e1->setActiveCollision(id2);
    e2->setActiveCollision(id1);
}

void EntryManager::resolvePairs(Entry* e, uint64_t ms, float delta) {
    const int id1 = e->ptr->getPlateId();
    const int n = (getTotalSize() + 31) >> 5;
    for (int w = 0; w < n; ++w) {
        uint32_t mask = e->active_collisions.at(w);
        while (mask != 0) {
            const int id2 = (w << 5) | std::countr_zero(mask);
            auto* ps = pairs_mgr_.get(id1, id2);
            const bool apart = ps->isApart(ms);
            if (ps->isStale(ms)) {
                ps->reset(apart);
                ps->cooldown(delta);
            }
            if (apart) { e->setInactiveCollision(id2); }
            mask &= mask - 1;
        }
    }
}

void EntryManager::sort(SortPass pass) {
    if (pass == SortPass::eStack) {
        sortBy<SortMode::eDefault, false>();
        return;
    }
    const guid_t target_guid = object_mgr::getTargetGuid();
    const RaiseFilter raise = mouseoverRaise();
    CGNamePlateFrame* focus = CGNamePlateFrame::focus;
    if (focus != nullptr) {
        const Entry* e = getEntry(CGNamePlateFrameExt::of(focus)->getPlateId());
        if (e == nullptr || e->ptr != focus || !raise.passes(e->hasState(Entry::State::eIsFriendly))) {
            focus = nullptr;
        }
    }
    if (target_guid != 0 && focus != nullptr) {
        sortBy<SortMode::eTargetFocus, true>(target_guid, focus, raise);
    } else if (target_guid != 0) {
        sortBy<SortMode::eTarget, true>(target_guid, nullptr, raise);
    } else if (focus != nullptr) {
        sortBy<SortMode::eFocus, true>(0, focus, raise);
    } else {
        sortBy<SortMode::eDefault, true>(0, nullptr, raise);
    }
}

void EntryManager::flushAdded() {
    LuaState* l = lua::getLuaState();
    const int n = (getTotalSize() + 63) / 64;
    for (int w = 0; w < n; ++w) {
        uint64_t word = pending_.at(w);
        while (word != 0) {
            const int index = w * 64 + std::countr_zero(word);
            Entry* e = &by_id_.at(index);
            const auto* unit = object_mgr::get<CGUnit_C>(e->ptr->owner_guid_, eTypemaskUnit);
            if (unit != nullptr && unit->nameplate_ != nullptr) {
                e->guid = unit->nameplate_->owner_guid_;
                lua::pushframe(l, e->ptr);
                lua::pushString(l, tokens_.at(index).c_str());
                lua::setField(l, -2, "unit");
                lua::pop(l, 1);
                kEventRegistry->fire(kEventUnitAdded, "%s", tokens_.at(index).c_str());
            }
            word &= word - 1;
        }
    }
}

void EntryManager::flushRemoved() {
    std::erase_if(entries_, [](Entry* e) {
        if (!e->hasState(Entry::State::eIsActive)) {
            e->guid = 0;
            return true;
        }
        return false;
    });
}

void EntryManager::appendAdded(int index) {
    if (index < 0 || index >= getTotalSize()) { return; }
    pending_.at(index / 64) |= (1ULL << (index % 64));
    if (Entry* e = &by_id_.at(index); !e->hasState(Entry::State::eIsActive)) {
        e->setState(Entry::State::eIsActive, true);
        entries_.push_back(e);
    }
}

Entry* EntryManager::initEntry(CGNamePlateFrameExt* plate) {
    const int index = plate->getPlateId();
    if (index == -1) {
        // plates live for one ui session, CSimpleTop deletes them right after wipeActive clears us
        by_id_.push_back({.ptr = plate});
        plate->setPlateId(getTotalSize() - 1);

        onPlateCreated(plate);
        return &by_id_.back();
    }
    if (index < 0 || index >= getTotalSize() || by_id_.at(index).ptr != plate) { return nullptr; }
    return &by_id_.at(index);
}

void EntryManager::markRemoved(Entry* e, int id) const {
    kEventRegistry->fire(kEventUnitRemoved, "%s", getToken(id));
    e->setState(Entry::State::eIsActive, false);
}

void EntryManager::onBound(CGNamePlateFrameExt* plate) {
    const int id = plate->getPlateId();
    // a plate never announced is track()'s
    if (id < 0 || id >= getTotalSize()) { return; }
    if (Entry& e = by_id_.at(id); e.ptr == plate && e.hasState(Entry::State::eIsActive)) {
        e.setState(Entry::State::eIsRebound, true);
    }
}

void EntryManager::track(CGNamePlateFrameExt* plate) {
    const int id = plate->getPlateId();
    Entry* e = getEntry(id);
    CGUnit_C* unit = plate->boundUnit();
    if (e != nullptr && e->hasState(Entry::State::eIsActive)) {
        if (unit == nullptr) {
            markRemoved(e, id);
        } else if (e->hasState(Entry::State::eIsRebound)) {
            kEventRegistry->fire(kEventUnitRemoved, "%s", getToken(id));
            bindEntry(e, unit);
        }
        return;
    }

    if (unit == nullptr) { return; }
    e = initEntry(plate);
    if (e == nullptr) { return; }
    bindEntry(e, unit);
}

void EntryManager::bindEntry(Entry* e, CGUnit_C* unit) {
    // a rebound entry is still listed in entries_, appendAdded must not list it twice
    const bool listed = e->hasState(Entry::State::eIsActive);
    e->clearState();
    e->setState(Entry::State::eIsActive, listed);
    e->setState(Entry::State::eIsFriendly, unit->isFriendly());
    e->classification = unit->getCreatureRank();

    e->updateStacking();
    e->updateClamping();
    resolvePairs(e, static_cast<uint64_t>(-1), 0);
    appendAdded(e->ptr->getPlateId());

    // for occlusion
    e->ptr->setFresh(true);
    e->ptr->setOpaque(false);
}

void EntryManager::stack(uint64_t ms, float scene_time, float pull_scale) {
    const Settings& s = *kSettings;
    if (s.stackingMode() == StackingMode::eDisabled) { return; }
    const float band_x = s.band_x;
    const float band_y = s.band_y;
    const auto n = std::ssize(entries_);

    for (std::ptrdiff_t i = 0; i < n; ++i) {
        Entry* e1 = entries_.at(i);
        if (!e1->hasState(Entry::State::eShouldStack) || e1->hasState(Entry::State::eIsFresh) || e1->isTransparent()) {
            continue;
        }
        const bool frozen1 = e1->hasState(Entry::State::eIsFrozen);
        bool freed = frozen1 || s.stacking > static_cast<int>(StackingMode::eDisabled);

        for (std::ptrdiff_t j = i + 1; j < n; ++j) {
            Entry* e2 = entries_.at(j);
            // skip fresh plates, UNIT_ADDED callbacks later might disable collisions
            if (!e2->hasState(Entry::State::eShouldStack) || e2->hasState(Entry::State::eIsFresh) ||
                e2->isTransparent()) {
                continue;
            }
            const bool frozen2 = e2->hasState(Entry::State::eIsFrozen);
            if (frozen1 && frozen2) { continue; }  // both pinned, neither can yield
            seedPair(e1, e2, ms);
            auto* ps = getPair(e1, e2);
            const float dx = e1->getReqDxFor(e2);
            const float min_sep_x = e1->getAvgWFor(e2, band_x * ps->hysteresis);
            if (!(dx <= min_sep_x - kEps4f)) { continue; }

            if (float req_y = e1->getReqYFor(e2, band_y); req_y > e2->target_offset.y) {
                if (frozen2) { continue; }
                if (!frozen1 && e2->getRankWeight() > e1->getRankWeight()) {
                    // rank prio hot correction
                    req_y = e2->getReqYFor(e1, band_y);
                    if (req_y > e1->target_offset.y) {
                        e1->target_offset.y = req_y;
                        commitPair(e1, e2, ms, scene_time, band_y, band_x);
                    }
                } else {
                    if (!freed && !e1->resolvePush(e2, band_y, ps->hysteresis)) {
                        freed = true;
                        continue;
                    }
                    e2->target_offset.y = req_y;
                    e1->pull(e2, dx, band_x, pull_scale);
                    commitPair(e1, e2, ms, scene_time, band_y, band_x);
                }
            } else if (!ps->isStale(1) &&
                (e1->getTopNdc(ps->hysteresis) + e1->target_offset.y + e1->getAvgHFor(e2, band_y) >
                    (e2->getBotNdc(ps->hysteresis) + e2->target_offset.y))) {
                // overlap at extended range, keep the commitment and pull up
                e1->pull(e2, dx, band_x, pull_scale);
                commitPair(e1, e2, ms, scene_time, band_y, band_x);
            }
        }
    }
}

void EntryManager::onPlateCreated(CGNamePlateFrameExt* plate) {
    LuaState* l = lua::getLuaState();
    lua::pushString(l, kEventCreated);
    lua::pushframe(l, plate);
    for (const auto& [name, func] : kMethods) {
        lua::pushCFunction(l, func);
        lua::setField(l, -2, name);
    }

    // event name and frame are the two args already on the stack
    kEventRegistry->signal(kEventCreated, l, 2);
    lua::pop(l, 2);
}
}  // namespace name_plates
