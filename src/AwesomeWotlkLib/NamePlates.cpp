#include "NamePlates.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "Extensions.h"
#include "NamePlateManager.h"
#include "NamePlateSettings.h"
#include "WorldCull.h"

#include "include/Camera/CGCamera.h"
#include "include/FrameScript/FrameScript.h"
#include "include/Game/CGGameUI.h"
#include "include/Game/CGInputControl.h"
#include "include/Lib/Lua.h"
#include "include/Math/Math.h"
#include "include/ObjectManager/CGUnit_C.h"
#include "include/ObjectManager/ObjectManager.h"
#include "include/UI/CGNamePlateFrame.h"
#include "include/UI/CLayoutFrame.h"
#include "include/UI/UIBindings.h"
#include "include/World/CGWorldFrame.h"

using namespace name_plates;

namespace {
using extensions::console::kEventRegistry;

// whether an off-screen plate of this unit is kept alive and clamped
bool clampsUnit(CGUnit_C* unit) {
    const Settings& s = *kSettings;
    return s.clampMode() != ClampMode::eDisabled &&
        s.clampFilterPasses(unit->getGuid(), object_mgr::getTargetGuid(), game_ui::inCombatLockdown()) &&
        s.clampsRank([unit] { return unit->getCreatureRank(); }) &&
        s.clampsAffiliation([unit] { return unit->isFriendly(); });
}

// full replacement
HOOKKIT_CALLSITE_HOOK(updateNamePlatePositions_call, 0x004F90DD, CGWorldFrame::updateNamePlatePositions, 10, 8);
HOOKKIT_BIND(updateNamePlatePositions_call, [](CGWorldFrame* self) {
    kPlates->update(self);
});

HOOKKIT_BIND(CGUnit_C::updateReaction_hook, [](CGUnit_C* self, char update_all) {
    const int result = self->updateReaction(update_all);
    kPlates->onReactionChanged(self);
    return result;
});

// full replacement; the engine's own caller cleans the stack, see the nop in initialize
HOOKKIT_BIND(CGUnit_C::isVisible_hook, [](CGUnit_C* self, CGWorldFrame* wf, Vec3f* out) {
    if (self->nameplate_ == nullptr && !self->readyForPlate()) { return false; }

    const Settings& s = *kSettings;
    Vec3f world_pos;
    if (self->nameChainPlaced()) {
        self->getNamePosition(world_pos);
    } else {
        // original fallback without a model: 1.25 heights above the position
        self->getPosition(world_pos);
        world_pos.z += self->height_ * self->scale_ * 1.25f;
    }
    world_pos.z += s.placement;
    const auto [mask, behind, depth] = wf->projectName(world_pos, *out);
    if (!behind && mask == 15) { return true; }
    if (!behind && mask > 0) {
        const auto within = [wf, out](Vec2f ndc_margin) {
            const Vec2f margin = math::ndcToDdc(ndc_margin);
            return (out->x >= -margin.x) && (out->x <= (wf->rect_.right - wf->rect_.left) + margin.x) &&
                (out->y >= -margin.y) && (out->y <= (wf->rect_.top - wf->rect_.bottom) + margin.y);
        };
        constexpr Vec2f kHalfPlate{CGNamePlateFrame::kDefaultWidth * 0.5f, CGNamePlateFrame::kDefaultHeight * 0.5f};
        // >=50% out of the viewport but still visible, or near enough that the static name would still reach in: keep
        // it, prevents static names popping in
        if (within(kHalfPlate) || within(wf->nameHalfExtent(self, depth, s.placement))) { return true; }
    }
    if (!clampsUnit(self)) { return false; }
    switch (s.clampMode()) {
        case ClampMode::eAll:
        case ClampMode::eBoss:
            return mask == 7;
        case ClampMode::eAllEdges:
        case ClampMode::eBossEdges:
            return mask > 0;
        case ClampMode::eDisabled:
        default:
            return false;
    }
});

// full replacement
template <FreezeMode Mode, FreezeAffiliation Affiliation>
void pickFocus(Vec3f* pos) {
    const CGInputControl* input = CGInputControl::get{}();
    if (input == nullptr) { return; }

    // the engine holds focus still under a mouse button, the pre-press pick stays the plate pressed on
    CGNamePlateFrame*& focus = CGNamePlateFrame::focus;
    if ((input->control_flags_ & CGInputControl::kMouseBusyFlags) == 0) {
        const MouseMode mode = derived_settings.mouse_mode;
        CGNamePlateFrame* prio;
        if (hasFlag(mode, MouseMode::eClickThruEnemy)) {
            prio = kPlates->pickPlate<ClickLogic::eThruEnemy>(pos);
        } else if (hasFlag(mode, MouseMode::eClickThruFriend)) {
            prio = kPlates->pickPlate<ClickLogic::eThruFriend>(pos);
        } else {
            prio = kPlates->pickPlate<ClickLogic::eNone>(pos);
        }

        if (prio != focus) {
            if (focus != nullptr) { focus->onFocusLost(); }
            focus = prio;
            if (prio != nullptr) { prio->onFocusGained(); }
            markDirty();
        }
    }
    if constexpr (Mode != FreezeMode::eDisabled) {
        if constexpr (freezesInCombatOnly(Mode)) {
            if (!game_ui::inCombatLockdown()) { return; }
        }
        kPlates->setHover<freezesTarget(Mode), Affiliation>(focus, pos);
    }
}

// focus updates stop while a mouse button is held, a camera drag would keep the highlight
HOOKKIT_BIND(CGInputControl::handleMouseMoveRelative_hook, [](CGInputControl* self, void* event) {
    self->handleMouseMoveRelative(event);
    if (!self->isDragging(CGInputControl::input_event_time)) { return; }
    if (CGNamePlateFrame*& focus = CGNamePlateFrame::focus; focus != nullptr) {
        focus->onFocusLost();
        focus = nullptr;
        markDirty();
    }
});

// fires on combat mode changes; forces a pass even when everything is perfectly stationary
HOOKKIT_BIND(CGUIBindings::setBindingModeState_hook, [](CGUIBindings* self, int mode_index, int enabled) {
    markDirty();
    self->setBindingModeState(mode_index, enabled);
});

HOOKKIT_BIND(game_ui::target_hook, [](guid_t guid) {
    game_ui::target{}(guid);
    markDirty();
});

HOOKKIT_BIND(game_ui::clearTarget_hook, [](guid_t guid, int flag) {
    game_ui::clearTarget{}(guid, flag);
    markDirty();
});

// the constructor leaves the bytes the ext state lives in untouched, reused blocks would keep stale ids
HOOKKIT_BIND(CGNamePlateFrame::ctor_hook, [](CGNamePlateFrame* self, CSimpleFrame* parent) {
    CGNamePlateFrame* result = self->ctor(parent);
    CGNamePlateFrameExt::of(result)->reset();
    return result;
});

HOOKKIT_BIND(CGNamePlateFrame::init_hook, [](CGNamePlateFrame* self, CGUnit_C* unit) {
    self->onFocusLost();
    self->init(unit);
    kPlates->onBound(CGNamePlateFrameExt::of(self));
});

HOOKKIT_BIND(CGNamePlateFrame::wipeActive_hook, [] {
    kPlates->clearAll();
    return CGNamePlateFrame::wipeActive{}();
});

HOOKKIT_BIND(CGNamePlateFrame::wipeCDA_hook, [] {
    kPlates->clearAll();
    return CGNamePlateFrame::wipeCDA{}();
});

// skips the engine's own frame level assignment, levels are set in updateNamePlatePositions
HOOKKIT_NAMED_BIND_RAW(plateLevel_site, 0x0098E9F9, 0x0098EA27) {
    constexpr uintptr_t kJmpback = plateLevel_site::target();
    return CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(jmpTo(kJmpback, pushReg(Reg::eDi)));
};

HOOKKIT_NAMED_BIND_RAW(plateAlpha_site, 0x0098EAA7, 0x0098EAD2) {
    const uintptr_t alpha = HOOKKIT_LAMBDA_ADDR([](CGUnit_C* unit) -> uint8_t {
        if (unit == nullptr || unit->nameplate_ == nullptr) { return 255; }
        const Settings& s = *kSettings;
        const Derived& d = derived_settings;
        CGNamePlateFrameExt* plate = CGNamePlateFrameExt::of(unit->nameplate_);

        const guid_t target_guid = object_mgr::getTargetGuid();
        const bool is_target_or_mouse_over =
            target_guid != 0 ? unit->getGuid() == target_guid : plate == CGNamePlateFrame::focus;
        uint8_t target_alpha = 255;
        if (target_guid != 0 && !is_target_or_mouse_over) { target_alpha = d.non_target_alpha; }

        if (!is_target_or_mouse_over && d.occlusion_on &&
            (static_cast<OcclusionMode>(s.occlusion_mode) == OcclusionMode::eAlways || !game_ui::inCombatLockdown()) &&
            !plate->isOpaque()) {
            if (CGCamera* camera = CGCamera::get{}()) {
                Vec3f hit_point;
                float dist = 1.0f;
                Vec3f start = camera->pos_;
                Vec3f end;
                unit->getPosition(end);
                end.z += unit->movement_.bounding_height_ * 0.666f;

                if (math::trace(start, end, CGWorldFrame::kWorldTraceHitFlags, hit_point, dist)) {
                    if (d.occlude_up) {
                        target_alpha = std::min(target_alpha, d.occlusion_cap);
                    } else {
                        target_alpha = static_cast<uint8_t>(static_cast<float>(target_alpha) * d.occlusion_scale);
                    }
                }
            }
        }

        if (plate->isFresh()) {
            plate->setFresh(false);
            return target_alpha;
        }

        const auto current = static_cast<float>(plate->alpha_self_);
        const float delta = static_cast<float>(target_alpha) - current;
        if (std::abs(delta) < 0.5f) { return target_alpha; }

        const CGWorldFrame* wf = CGWorldFrame::get();
        const float step = delta * perFrame(s.alpha_speed, wf != nullptr ? sceneDelta(wf) : 1.0f / kTunedFps);
        const float next_alpha = current + (std::abs(step) < 1.0f ? std::copysign(1.0f, delta) : step);

        if (std::abs(static_cast<float>(target_alpha) - next_alpha) < 1.0f) { return target_alpha; }
        const bool overshoot =
            (delta > 0.0f && next_alpha > target_alpha) || (delta < 0.0f && next_alpha < target_alpha);
        return overshoot ? target_alpha : static_cast<uint8_t>(next_alpha);
    });
    constexpr uintptr_t kJmpback = plateAlpha_site::target();
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .step(pushReg(Reg::eDi))
        .build(alpha, jmpTo(kJmpback, pushReg(Reg::eAx)));
};
}  // namespace

namespace {
namespace views = world_cull::views;

HOOKKIT_CALLSITE_HOOK(plateGateFirst_call, 0x0072B094, CMapEntity::isDrawn);
HOOKKIT_CALLSITE_HOOK(plateGateSecond_call, 0x0072B2B5, CMapEntity::isDrawn);
HOOKKIT_CALLSITE_HOOK(plateGateUpdate_call, 0x00734504, CMapEntity::isDrawn);

bool wantsClampView(CMapEntity* entity) {
    if (entity->owner_guid_ == 0 || CMapEntity::isDrawn{}(entity) != 0) { return false; }
    auto* unit = object_mgr::get<CGUnit_C>(entity->owner_guid_, eTypemaskUnit);
    return unit != nullptr && unit->inPlateRange() && clampsUnit(unit) && unit->plateKindShown();
}

constexpr views::Spec clampView(views::Side side, const Rectf& rect) {
    return {.side = side, .rect = rect, .screen = true, .wants = wantsClampView, .animates = true};
}

constexpr auto kClampViews = std::to_array<views::Spec>({
    clampView(views::Side::eUp, {-1.0f, -1.0f, 0.0f, 1.0f}),
    clampView(views::Side::eLeft, {-1.0f, 0.0f, 1.0f, 1.0f}),
    clampView(views::Side::eRight, {-1.0f, -1.0f, 1.0f, 0.0f}),
});

constexpr utils::Accessor<std::vector<views::Id>, struct OpenClampViewsTag> kOpenClampViews;

int plateDrawn(CMapEntity* entity) {
    if (CMapEntity::isDrawn{}(entity) != 0) { return 1; }
    return std::ranges::any_of(*kOpenClampViews, [entity](views::Id id) { return views::reached(id, entity); }) ? 1 : 0;
}

void applyClampMode(int, bool changed) {
    if (!changed) { return; }
    const Settings& s = *kSettings;
    const ClampViews clamp_views = s.clampViews();
    size_t wanted = 0;
    if (s.clampMode() != ClampMode::eDisabled && clamp_views != ClampViews::eDisabled) {
        wanted = clamp_views == ClampViews::eTopSides && s.clampAllEdges() ? kClampViews.size() : 1;
    }
    auto& ids = *kOpenClampViews;
    while (ids.size() > wanted) {
        views::close(ids.back());
        ids.pop_back();
    }
    while (ids.size() < wanted) {
        ids.push_back(views::open(kClampViews[ids.size()]));
    }
    const bool on = wanted != 0;
    static_cast<void>(hookkit::HookTransaction::reinstallIf<plateDrawn>(plateGateFirst_call{}, on));
    static_cast<void>(hookkit::HookTransaction::reinstallIf<plateDrawn>(plateGateSecond_call{}, on));
    static_cast<void>(hookkit::HookTransaction::reinstallIf<plateDrawn>(plateGateUpdate_call{}, on));
    kPlates->refreshClamping();
    markDirty();
}

void applyStackingMode(int, bool changed) {
    if (!changed) { return; }
    kPlates->refreshStacking();
    markDirty();
}

void applyDerived() {
    derived_settings = Derived::make(*kSettings);
    markDirty();
}

using FocusHook = CGUnit_C::setNamePlateFocus_hook;

template <FreezeMode Mode>
void* focusDetour(FreezeAffiliation affiliation) {
    switch (affiliation) {
        case FreezeAffiliation::eEnemy:
            return FocusHook::staticDetour<pickFocus<Mode, FreezeAffiliation::eEnemy>>();
        case FreezeAffiliation::eFriendly:
            return FocusHook::staticDetour<pickFocus<Mode, FreezeAffiliation::eFriendly>>();
        case FreezeAffiliation::eAll:
        default:
            return FocusHook::staticDetour<pickFocus<Mode, FreezeAffiliation::eAll>>();
    }
}

// the focus hook is always attached, the freeze cvars only pick its variant
void applyFreeze() {
    const Settings& s = *kSettings;
    const auto affiliation = static_cast<FreezeAffiliation>(s.freeze_affiliation);
    void* detour;
    switch (static_cast<FreezeMode>(s.freeze_mode)) {
        case FreezeMode::eAlways:
            detour = focusDetour<FreezeMode::eAlways>(affiliation);
            break;
        case FreezeMode::eCombat:
            detour = focusDetour<FreezeMode::eCombat>(affiliation);
            break;
        case FreezeMode::eAlwaysNoTarget:
            detour = focusDetour<FreezeMode::eAlwaysNoTarget>(affiliation);
            break;
        case FreezeMode::eCombatNoTarget:
            detour = focusDetour<FreezeMode::eCombatNoTarget>(affiliation);
            break;
        case FreezeMode::eDisabled:
        default:
            detour = FocusHook::staticDetour<pickFocus<FreezeMode::eDisabled, FreezeAffiliation::eAll>>();
            break;
    }
    static_cast<void>(hookkit::HookTransaction::reinstall(FocusHook{}, detour));
    markDirty();
}

void applyDistance(float distance, bool changed) {
    if (!changed) { return; }
    game_ui::nameplate_max_distance_sq = distance * distance;
    markDirty();
}

// numeric guid string at arg 1, 0 if missing
guid_t guidArg(LuaState* l) {
    const char* guid_str = lua::checkString(l, 1);
    return guid_str != nullptr ? std::strtoull(guid_str, nullptr, 0) : 0;
}

int pushPlate(LuaState* l, guid_t guid) {
    CGNamePlateFrame* plate = kPlates->getPlate(guid);
    if (plate == nullptr) { return 0; }
    lua::pushframe(l, plate);
    return 1;
}

int luaGetNamePlates(LuaState* l) {
    lua::createTable(l, 0, 0);
    int id = 1;
    for (const Entry* e : kPlates->get()) {
        lua::pushframe(l, e->ptr);
        lua::rawSetI(l, -2, id++);
    }
    return 1;
}

int luaGetNamePlateForUnit(LuaState* l) {
    const char* token = lua::checkString(l, 1);
    return token != nullptr ? pushPlate(l, object_mgr::string2Guid(token)) : 0;
}

int luaGetNamePlateByGuid(LuaState* l) { return pushPlate(l, guidArg(l)); }

int luaGetNamePlateTokenByGuid(LuaState* l) {
    const guid_t guid = guidArg(l);
    if (guid == 0) { return 0; }
    lua::pushString(l, kPlates->getToken(guid));
    return 1;
}

int luaOpenNamePlates(LuaState* l) {
    static constexpr std::array<lua::LuaLReg, 4> kFuncs = {{
        {.name = "GetNamePlates", .func = luaGetNamePlates},
        {.name = "GetNamePlateForUnit", .func = luaGetNamePlateForUnit},
        {.name = "GetNamePlateByGUID", .func = luaGetNamePlateByGuid},
        {.name = "GetNamePlateTokenByGUID", .func = luaGetNamePlateTokenByGuid},
    }};
    lua::createTable(l, 0, kFuncs.size());
    for (const auto& [name, func] : kFuncs) {
        lua::pushCFunction(l, func);
        lua::setField(l, -2, name);
    }
    lua::setGlobal(l, "C_NamePlate");
    return 0;
}
}  // namespace

guid_t name_plates::getTokenGuid(int id) { return kPlates->getTokenGuid(id); }

int name_plates::getTokenId(guid_t guid) { return kPlates->getTokenId(guid); }

void name_plates::initialize(hookkit::HookTransaction& tx) {
    auto& cvars = *extensions::console::kCvarRegistry;

    kPlates->init(kMaxPlates);

    extensions::console::kLuaLibRegistry->add(luaOpenNamePlates);
    kEventRegistry->add(kEventCreated);
    kEventRegistry->add(kEventUnitAdded);
    kEventRegistry->add(kEventUnitRemoved);

    constexpr auto kHitboxTop = static_cast<int>(CLayoutFrame::eHbTop);
    constexpr auto kHitboxBottom = static_cast<int>(CLayoutFrame::eHbBottom);
    constexpr auto kStackingMax = static_cast<int>(StackingMode::eFriendly);
    constexpr auto kClampMax = static_cast<int>(ClampMode::eBossEdges);
    constexpr auto kClampFilterMax = static_cast<int>(ClampFilter::eTargetCombat);
    constexpr auto kClampAffiliationMax = static_cast<int>(ClampAffiliation::eFriendly);
    constexpr auto kClampViewsMax = static_cast<int>(ClampViews::eTopSides);
    constexpr auto kFreezeMax = static_cast<int>(FreezeMode::eCombatNoTarget);
    constexpr auto kFreezeAffiliationMax = static_cast<int>(FreezeAffiliation::eFriendly);
    constexpr auto kMouseModeMax = static_cast<int>(kMouseModeMap.size()) - 1;

    cvars.add<float>({
        .name = "nameplateDistance",
        .init = 41.0f,
        .min{41.0f},
        .max{200.0f},
        .on_change = applyDistance
    });
    cvars.add<float>({
        .name = "nameplatePlacement",
        .init = 0.0f,
        .min{-1.0f},
        .max{2.0f},
        .fmt = "%.4f",
        .on_change = markDirty
    });
    cvars.add<int>({
        .name = "nameplateMouseMode",
        .init = 0,
        .min{0},
        .max{kMouseModeMax},
        .on_change = applyDerived
    });
    cvars.add<float>({
        .name = "nameplateBandX",
        .init = 0.7f,
        .min{0.1f},
        .max{1.0f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateBandY",
        .init = 1.0f,
        .min{0.1f},
        .max{1.5f},
        .on_change = markDirty
    });
    cvars.add<int>({
        .name = "nameplateHitboxAnchor",
        .init = 1,
        .min{kHitboxTop},
        .max{kHitboxBottom},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateHitboxWidthE",
        .init = 1.0f,
        .min{0.0f},
        .max{1.0f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateHitboxHeightE",
        .init = 1.0f,
        .min{0.0f},
        .max{1.0f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateHitboxWidthF",
        .init = 1.0f,
        .min{0.0f},
        .max{1.0f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateHitboxHeightF",
        .init = 1.0f,
        .min{0.0f},
        .max{1.0f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateRaiseSpeed",
        .init = 100.0f,
        .min{1.0f},
        .max{250.0f},
        .on_change = applyDerived
    });
    cvars.add<float>({
        .name = "nameplateLowerSpeed",
        .init = 100.0f,
        .min{1.0f},
        .max{250.0f},
        .on_change = applyDerived
    });
    cvars.add<float>({
        .name = "nameplatePullSpeed",
        .init = 50.0f,
        .min{1.0f},
        .max{250.0f},
        .on_change = applyDerived
    });
    cvars.add<float>({
        .name = "nameplateRaiseDistance",
        .init = 8.0f,
        .min{1.0f},
        .max{20.0f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplatePullDistance",
        .init = 0.25f,
        .min{0.0f},
        .max{0.75f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateOcclusionAlpha",
        .init = 1.0f,
        .min{-1.0f},
        .max{1.0f},
        .on_change = applyDerived
    });
    cvars.add<int>({
        .name = "nameplateOcclusionMode",
        .init = 0,
        .min{0},
        .max{1},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateNonTargetAlpha",
        .init = 0.5f,
        .min{0.0f},
        .max{1.0f},
        .on_change = applyDerived
    });
    cvars.add<float>({
        .name = "nameplateAlphaSpeed",
        .init = 0.25f,
        .min{0.01f},
        .max{1.0f},
        .fmt = "%.3f",
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateInertia",
        .init = 1.0f,
        .min{0.0f},
        .max{20.0f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateHysteresisDecay",
        .init = 1.0f,
        .min{0.25f},
        .max{30.0f},
        .on_change = markDirty
    });
    cvars.add<int>({
        .name = "nameplateClampMode",
        .init = 0,
        .min{0},
        .max{kClampMax},
        .on_change = applyClampMode
    });
    cvars.add<float>({
        .name = "nameplateClampModeVOffset",
        .init = 0.01f,
        .min{0.0f},
        .max{0.05f},
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateClampModeHOffset",
        .init = 0.01f,
        .min{0.0f},
        .max{0.1f},
        .on_change = markDirty
    });
    cvars.add<int>({
        .name = "nameplateClampModeFilter",
        .init = 0,
        .min{0},
        .max{kClampFilterMax},
        .on_change = markDirty
    });
    cvars.add<int>({
        .name = "nameplateClampModeAffiliation",
        .init = 0,
        .min{0},
        .max{kClampAffiliationMax},
        .on_change = applyClampMode
    });
    cvars.add<int>({
        .name = "nameplateClampViews",
        .init = 0,
        .min{0},
        .max{kClampViewsMax},
        .on_change = applyClampMode
    });
    cvars.add<int>({
        .name = "nameplateMouseFreeze",
        .init = 0,
        .min{0},
        .max{kFreezeMax},
        .on_change = applyFreeze
    });
    cvars.add<int>({
        .name = "nameplateMouseFreezeAffiliation",
        .init = 0,
        .min{0},
        .max{kFreezeAffiliationMax},
        .on_change = applyFreeze
    });
    cvars.add<float>({
        .name = "nameplateMouseFreezeGrace",
        .init = 0.15f,
        .min{0.0f},
        .max{1.0f},
        .fmt = "%.3f",
        .on_change = markDirty
    });
    cvars.add<float>({
        .name = "nameplateMouseFreezeTime",
        .init = 0.3f,
        .min{0.0f},
        .max{2.0f},
        .fmt = "%.3f",
        .on_change = markDirty
    });
    cvars.add<int>({
        .name = "nameplateStacking",
        .init = 0,
        .min{-kStackingMax},
        .max{kStackingMax},
        .on_change = applyStackingMode
    });

    applyDerived();

    extensions::framescript::kTokenRegistry->add("nameplate", getTokenGuid, getTokenId);

    // the replaced isVisible cleans its own args, the caller's add esp, 8 would double it
    constexpr std::array<uint8_t, 3> kNop3 = {0x90, 0x90, 0x90};
    tx.patchBytes(reinterpret_cast<void*>(0x0072B2D0), kNop3.data(), kNop3.size());
    const float* placement = &cvars.ref<float>("nameplatePlacement");
    tx.forceWrite<const float*>(0x00715737 + 2, placement);

    FocusHook::bindStatic<pickFocus<FreezeMode::eDisabled, FreezeAffiliation::eAll>>();
    tx.attach(updateNamePlatePositions_call{}, CGUnit_C::isVisible_hook{}, CGUnit_C::updateReaction_hook{},
        FocusHook{}, CGInputControl::handleMouseMoveRelative_hook{},
        CGUIBindings::setBindingModeState_hook{}, game_ui::target_hook{},
        game_ui::clearTarget_hook{}, CGNamePlateFrame::ctor_hook{}, CGNamePlateFrame::init_hook{},
        CGNamePlateFrame::wipeActive_hook{},
        CGNamePlateFrame::wipeCDA_hook{}, plateLevel_site{}, plateAlpha_site{});
}
