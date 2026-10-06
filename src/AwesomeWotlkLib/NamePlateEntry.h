#pragma once

#include <array>
#include <cmath>
#include <cstdint>

#include "NamePlateSettings.h"

#include "include/Constants.h"
#include "include/Math/Primitives.h"
#include "include/UI/CGNamePlateFrame.h"

namespace name_plates {
class CGNamePlateFrameExt : public CGNamePlateFrame {
public:
    static CGNamePlateFrameExt* of(CGNamePlateFrame* plate) { return reinterpret_cast<CGNamePlateFrameExt*>(plate); }

    void reset() {
        setPlateId(-1);
        setOpaque(false);
        setFresh(false);
    }

    [[nodiscard]]
    int getPlateId() const {
        return static_cast<int>(_alignment) - 1;
    }

    void setPlateId(int id) { _alignment = static_cast<uint32_t>(id + 1); }

    // opts out of occlusion fading
    [[nodiscard]]
    bool isOpaque() const {
        return _pad[0] != 0;
    }

    void setOpaque(bool on) { _pad[0] = on ? 1 : 0; }

    // snaps to the target alpha on the next pass instead of fading in
    [[nodiscard]]
    bool isFresh() const {
        return _pad[1] != 0;
    }

    void setFresh(bool on) { _pad[1] = on ? 1 : 0; }
};

static_assert(sizeof(CGNamePlateFrameExt) == sizeof(CGNamePlateFrame));

// per axis: x pulls sideways, y raises/lowers
struct MotionParams {
    Vec2f speed;
    Vec2f gain;  // Derived step gain of speed
    float inertia{};
    float delta{};
    Vec2f max_offset;  // fraction of the plate size
    Vec2f ceil;        // clamp insets from the screen edges
    bool all_edges{};
    bool clamp{};  // eShouldClamp gated by the per-pass filter
};

struct alignas(64) Entry {
    enum class State : uint32_t {
        eNone = 0,
        eShouldStack = 0x1,
        eShouldClamp = 0x2,
        eIsFriendly = 0x4,
        eIsTransient = 0x8,
        eIsFresh = 0x10,
        eIsActive = 0x20,
        eIsFrozen = 0x40,
        eIsSliding = 0x80,
        eIsResync = 0x100,
    };

    CGNamePlateFrameExt* ptr = nullptr;
    guid_t guid = 0;
    CreatureRank classification = eRankNormal;

    Vec2f momentum;
    Vec2f commit_target;
    Vec2f target_offset;
    Vec2f smooth_target;
    Vec2f stack_offset;
    Vec2f shown_offset;  // stack_offset after the screen clamp
    float accum_x = 0.0f;
    int push_count = 0;

    Vec2f drawn;  // last position handed to setPoint
    Vec2f freeze_pos;
    Vec2f slide_from;  // offset from the unit at release, the slide moves with the camera
    uint64_t grace_end = 0;
    float slide_time = 0.0f;

    State state = State::eNone;

    std::array<uint32_t, kMaxPlates / 32> active_collisions{};

    [[nodiscard]]
    bool hasState(State flag) const {
        return (static_cast<uint32_t>(state) & static_cast<uint32_t>(flag)) != 0;
    }

    void setState(State flag, bool on) {
        const auto bits = static_cast<uint32_t>(state);
        const auto mask = static_cast<uint32_t>(flag);
        state = static_cast<State>(on ? (bits | mask) : (bits & ~mask));
    }

    void clearState() {
        momentum = commit_target = target_offset = smooth_target = stack_offset = shown_offset = Vec2f{};
        accum_x = 0.0f;
        push_count = 0;
        drawn = freeze_pos = slide_from = Vec2f{};
        grace_end = 0;
        slide_time = 0.0f;
        state = State::eIsFresh;
    }

    void setActiveCollision(int id) { active_collisions.at(id >> 5) |= (1u << (id & 31)); }

    void setInactiveCollision(int id) { active_collisions.at(id >> 5) &= ~(1u << (id & 31)); }

    [[nodiscard]]
    bool isTransparent() const {
        return ptr->alpha_self_ == 0;
    }

    // projected position plus the settled / visible offset
    [[nodiscard]]
    Vec2f tar() const {
        return ptr->ndc_proj_ + target_offset;
    }

    [[nodiscard]]
    Vec2f vis() const {
        return ptr->ndc_proj_ + shown_offset;
    }

    [[nodiscard]]
    Vec2f size() const {
        return ptr->dims_ * ptr->CLayoutFrame::scale_;
    }

    [[nodiscard]]
    float getTopNdc(float perc = 1.0f) const {
        return ptr->ndc_proj_.y + size().y * 0.5f * perc;
    }

    [[nodiscard]]
    float getBotNdc(float perc = 1.0f) const {
        return ptr->ndc_proj_.y - size().y * 0.5f * perc;
    }

    [[nodiscard]]
    float getAvgWFor(const Entry* e, float perc = 1.0f) const {
        return (size().x + e->size().x) * 0.5f * perc;
    }

    [[nodiscard]]
    float getAvgHFor(const Entry* e, float perc = 1.0f) const {
        return (size().y + e->size().y) * 0.5f * perc;
    }

    // y offset e needs to sit right above this plate
    [[nodiscard]]
    float getReqYFor(const Entry* e, float perc = 1.0f) const {
        return tar().y + getAvgHFor(e, perc) - e->ptr->ndc_proj_.y;
    }

    [[nodiscard]]
    float getReqXFor(const Entry* e) const {
        return tar().x - e->tar().x;
    }

    [[nodiscard]]
    float getReqDxFor(const Entry* e) const {
        return std::abs(getReqXFor(e));
    }

    [[nodiscard]]
    float getProximity(const Entry* e, float bx = 1.0f, float by = 1.0f) const {
        const Vec2f gap = tar() - e->tar();
        return std::clamp(
            std::min(std::abs(gap.x) / getAvgWFor(e, bx), std::abs(gap.y) / getAvgHFor(e, by)), 0.0f, 1.0f);
    }

    [[nodiscard]]
    constexpr int getRankWeight() const {
        switch (classification) {
            case eRankTrivial:
                return 0;
            case eRankNormal:
                return 1;
            case eRankRare:
                return 2;
            case eRankElite:
                return 3;
            case eRankRareelite:
                return 4;
            case eRankWorldboss:
                return 5;
        }
        return 0;
    }

    [[nodiscard]]
    bool resolvePush(const Entry* e, float sep, float hyst) const {
        const bool within_band = (e->tar().y > getBotNdc(sep * (hyst + 1.0f)) + target_offset.y) &&
            (e->tar().y < getTopNdc(sep * (hyst + 1.0f)) + target_offset.y);
        const bool crossing = e->getBotNdc() + e->stack_offset.y > getTopNdc() + target_offset.y &&
            e->getBotNdc() + e->target_offset.y < getTopNdc() + target_offset.y;
        return within_band || crossing;
    }

    [[nodiscard]]
    bool isAt(Vec2f offset) const {
        return std::abs(stack_offset.x - offset.x) < kEps4f && std::abs(stack_offset.y - offset.y) < kEps4f;
    }

    void freshState(float settle, float pull_distance);
    void updateStacking();
    void updateClamping();
    void updateReaction();
    void pull(Entry* e, float dx, float band_x, float scale) const;
    void updVis(const MotionParams& p);
    void updateFreeze(bool hovered, bool release_now, uint64_t ms, float grace);
    Vec2f place(float slide_duration, float delta);
};
}  // namespace name_plates
