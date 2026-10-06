#include "NamePlateEntry.h"

#include <algorithm>
#include <cmath>

#include "include/Math/Math.h"
#include "include/ObjectManager/CGUnit_C.h"
#include "include/ObjectManager/ObjectManager.h"
#include "include/ObjectManager/ObjectManagerEnums.h"

namespace name_plates {
// settle: per-frame decay of the previous offsets toward home
void Entry::freshState(float settle, float pull_distance) {
    if (hasState(State::eIsFrozen)) {
        target_offset = freeze_pos - ptr->ndc_proj_;
        accum_x = 0.0f;
        push_count = 0;
        return;
    }
    const float max_pull = size().x * pull_distance;
    accum_x += (0.0f - accum_x) * settle;
    target_offset.y += (0.0f - target_offset.y) * settle;
    target_offset.x =
        std::clamp((push_count > 1) ? accum_x / static_cast<float>(push_count) : accum_x, -max_pull, max_pull);
    push_count = 0;
}

void Entry::updateStacking() {
    if (hasState(State::eIsTransient)) {
        setState(State::eShouldStack, false);
        return;
    }
    const StackingMode mode = kSettings->stackingMode();
    bool should_stack = mode != StackingMode::eDisabled;
    if (mode == StackingMode::eFriendly) {
        should_stack = hasState(State::eIsFriendly);
    } else if (mode == StackingMode::eEnemy) {
        should_stack = !hasState(State::eIsFriendly);
    }
    setState(State::eShouldStack, should_stack);
}

void Entry::updateClamping() {
    if (auto* unit = object_mgr::get<CGUnit_C>(ptr->owner_guid_, eTypemaskUnit)) {
        setState(State::eShouldClamp, kSettings->clampsRank([unit] { return unit->getCreatureRank(); }));
    }
}

void Entry::updateReaction() {
    if (auto* unit = object_mgr::get<CGUnit_C>(ptr->owner_guid_, eTypemaskUnit)) {
        setState(State::eIsFriendly, unit->isFriendly());
        updateStacking();
    }
}

// sideways pull on e, weighted by horizontal overlap
void Entry::pull(Entry* e, float dx, float band_x, float scale) const {
    const float req_x = getReqXFor(e);
    if (push_count == 0 || std::signbit(target_offset.x) == std::signbit(req_x)) {
        e->accum_x += scale * req_x * std::pow(std::clamp(1.0f - (dx / getAvgWFor(e, band_x)), 0.0f, 1.0f), 1.5f);
        e->push_count++;
    }
}

void Entry::updVis(const MotionParams& p) {
    if (hasState(State::eIsFrozen)) {
        momentum = Vec2f{};
        shown_offset = stack_offset = smooth_target = commit_target = target_offset = freeze_pos - ptr->ndc_proj_;
        return;
    }
    const Vec2f max = size() * p.max_offset;
    target_offset = Rectf{0.0f, -max.x, max.y, max.x}.clamp(target_offset);
    if (hasState(State::eIsResync)) {
        setState(State::eIsResync, false);
        momentum = Vec2f{};
        stack_offset = smooth_target = commit_target = target_offset;
    }

    // the axes only couple through these two
    const bool resting = isAt({});
    const bool settled = isAt(target_offset);
    for (size_t i = 0; i < 2; ++i) {
        const float gap = target_offset[i] - commit_target[i];
        const float want = std::abs(gap) > kEps4f ? (gap > 0.0f ? 1.0f : -1.0f) : 0.0f;
        float& mom = momentum[i];
        if (resting) {
            mom = want;
        } else {
            const float rate = (want * mom < 0.0f) ? 1.0f : p.speed[i] * 0.025f;
            mom += (want - mom) * std::clamp(rate * p.inertia * p.delta, 0.0f, 1.0f);
        }

        if (!settled) {
            commit_target[i] +=
                (target_offset[i] - commit_target[i]) * std::clamp(10.0f * std::abs(mom) * p.delta, 0.0f, 1.0f);
        } else {
            commit_target[i] = target_offset[i];
            mom *= std::pow(0.1f, p.delta * kTunedFps);
        }

        smooth_target[i] +=
            (commit_target[i] - smooth_target[i]) * (1.0f - std::exp(-p.speed[i] * std::abs(mom) * p.delta));

        const float step = perFrame(p.gain[i] * std::abs(mom * mom * mom), p.delta);
        const float d = smooth_target[i] - stack_offset[i];
        stack_offset[i] = std::abs(d) > kEps4f ? stack_offset[i] + d * std::clamp(step, 0.0f, 1.0f) : smooth_target[i];
    }

    // display only, keeps the clamp out of the animated state
    shown_offset = stack_offset;
    if (p.clamp) {
        // anchored at the top center, screen extents follow the aspect ratio
        const Vec2f screen = math::aspectNormal();
        const Vec2f dims = size();
        Rectf lim{-10000.0f, -10000.0f, screen.y - p.ceil.y - ptr->ndc_proj_.y, 10000.0f};
        if (p.all_edges) {
            lim.min_y = p.ceil.y + dims.y - ptr->ndc_proj_.y;
            lim.min_x = p.ceil.x + dims.x * 0.5f - ptr->ndc_proj_.x;
            lim.max_x = screen.x - p.ceil.x - dims.x * 0.5f - ptr->ndc_proj_.x;
        }
        shown_offset = lim.clamp(stack_offset);
    }
}

void Entry::updateFreeze(bool hovered, bool release_now, uint64_t ms, float grace) {
    if (hovered && !hasState(State::eIsFresh)) {
        if (!hasState(State::eIsFrozen)) {
            freeze_pos = drawn;  // mid-slide too, picks up where it is
            setState(State::eIsFrozen, true);
            setState(State::eIsSliding, false);
        }
        grace_end = ms + static_cast<uint64_t>(grace * 1000.0f);
        return;
    }
    if (!hasState(State::eIsFrozen) || (!release_now && ms < grace_end)) { return; }

    // stacking resumes from the pinned spot, restarting from home would lose its place in the stack
    momentum = Vec2f{};
    slide_from = freeze_pos - ptr->ndc_proj_;
    commit_target = target_offset = smooth_target = stack_offset = slide_from;
    accum_x = 0.0f;
    push_count = 0;
    slide_time = 0.0f;
    setState(State::eIsFrozen, false);
    setState(State::eIsSliding, true);
    setState(State::eIsResync, true);
}

Vec2f Entry::place(float slide_duration, float delta) {
    Vec2f pos = vis();
    if (hasState(State::eIsSliding)) {
        slide_time += delta;
        const float t = slide_duration > 0.0f ? std::min(slide_time / slide_duration, 1.0f) : 1.0f;
        if (t >= 1.0f) {
            setState(State::eIsSliding, false);
        } else {
            pos = ptr->ndc_proj_ + slide_from + (shown_offset - slide_from) * (t * t * (3.0f - 2.0f * t));
        }
    }
    drawn = pos;
    return pos;
}
}  // namespace name_plates
