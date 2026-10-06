#include "NamePlates.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>

#include "Extensions.h"
#include "NamePlateManager.h"
#include "NamePlateSettings.h"

#include "include/Camera/CGCamera.h"
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
    if (s.clampMode() == ClampMode::eDisabled) { return false; }
    return s.clampFilterPasses(unit->getGuid(), object_mgr::getTargetGuid(), game_ui::inCombatLockdown()) &&
        s.clampsRank([unit] { return unit->getCreatureRank(); });
}

// full replacement; the caller discards the return value
HOOKKIT_BIND(CGWorldFrame::updateNamePlatePositions_hook, [](CGWorldFrame* self) { kPlates->update(self); });

HOOKKIT_BIND(CGUnit_C::updateReaction_hook, [](CGUnit_C* self, char update_all) {
    const int result = self->updateReaction(update_all);
    kPlates->onReactionChanged(self);
    return result;
});

// full replacement; the engine's own caller cleans the stack, see the nop in initialize
HOOKKIT_BIND(CGUnit_C::isVisible_hook, [](CGUnit_C* self, CGWorldFrame* wf, Vec3f* out) {
    Vec3f world_pos;
    self->getNamePosition(world_pos);
    world_pos.z += kSettings->placement;
    int mask = 0;
    if (wf->getScreenCoords(&world_pos, out, &mask)) { return true; }
    if (mask > 0) {
        constexpr Vec2f kHalfPlate{CGNamePlateFrame::kDefaultWidth * 0.5f, CGNamePlateFrame::kDefaultHeight * 0.5f};
        const Vec2f margin = math::ndcToDdc(kHalfPlate);
        // >=50% out of the viewport but still visible: keep it, prevents static names popping in
        if ((out->x >= -margin.x) && (out->x <= (wf->rect_.right - wf->rect_.left) + margin.x) &&
            (out->y >= -margin.y) && (out->y <= (wf->rect_.top - wf->rect_.bottom) + margin.y)) {
            return true;
        }
    }
    if (!clampsUnit(self)) { return false; }
    switch (kSettings->clampMode()) {
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
HOOKKIT_BIND(CGUnit_C::setNamePlateFocus_hook, [](Vec3f* pos) {
    CGInputControl* input = CGInputControl::get{}();
    if (input == nullptr || (input->control_flags_ & CGInputControl::kMouseBusyFlags) != 0) { return; }

    const MouseMode mode = derived_settings.mouse_mode;
    CGNamePlateFrame* prio;
    if (hasFlag(mode, MouseMode::eClickThruEnemy)) {
        prio = kPlates->pickPlate<ClickLogic::eThruEnemy>(pos);
    } else if (hasFlag(mode, MouseMode::eClickThruFriend)) {
        prio = kPlates->pickPlate<ClickLogic::eThruFriend>(pos);
    } else {
        prio = kPlates->pickPlate<ClickLogic::eNone>(pos);
    }

    if (CGNamePlateFrame*& focus = CGNamePlateFrame::focus; prio != focus) {
        if (focus != nullptr) { focus->onFocusLost(); }
        focus = prio;
        if (prio != nullptr) { prio->onFocusGained(); }
        markDirty();
    }
    if (kSettings->freezeActive()) { kPlates->setHover(kPlates->hoverPlate(prio, pos)); }
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

HOOKKIT_BIND(CGNamePlateFrame::wipeActive_hook, [] {
    kPlates->clearAll();
    return CGNamePlateFrame::wipeActive{}();
});

HOOKKIT_BIND(CGNamePlateFrame::wipeCDA_hook, [] {
    kPlates->clearAll();
    return CGNamePlateFrame::wipeCDA{}();
});

// skips the engine's own frame level assignment, levels are set in updateNamePlatePositions
HOOKKIT_NAMED_BIND_RAW(plateLevel_site, 0x0098E9F9, {"jmpback", 0x0098EA27}) {
    constexpr uintptr_t kJmpback = plateLevel_site::target("jmpback");
    return CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(jmpTo(kJmpback, pushReg(Reg::eDi)));
};

// skips clearing render_dirty_flags_ after updateNamePlatePositions
HOOKKIT_NAMED_BIND_RAW(worldRenderDirty_site, 0x004F90E2, {"jmpback", 0x004F90EC}) {
    constexpr uintptr_t kJmpback = worldRenderDirty_site::target("jmpback");
    return CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(jmpTo(kJmpback, addReg(Reg::eSp, 8)));
};

HOOKKIT_NAMED_BIND_RAW(plateAlpha_site, 0x0098EAA7, {"jmpback", 0x0098EAD2}) {
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
    constexpr uintptr_t kJmpback = plateAlpha_site::target("jmpback");
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .step(pushReg(Reg::eDi))
        .build(alpha, jmpTo(kJmpback, pushReg(Reg::eAx)));
};
}  // namespace

namespace {
void applyClampMode(int, bool changed) {
    if (!changed) { return; }
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

void applyDistance(float distance, bool changed) {
    if (!changed) { return; }
    const float distance_sq = distance * distance;
    CGNamePlateFrame::max_distance_sq = distance_sq;
    hookkit::forceWrite<float>(game_ui::kTabTargetRangeCapSqAddr, distance_sq);
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
    constexpr auto kFreezeMax = static_cast<int>(FreezeMode::eCombat);
    constexpr auto kMouseModeMax = static_cast<int>(kMouseModeMap.size()) - 1;

    cvars.add<float>(
        {.name = "nameplateDistance", .init = 41.0f, .min{41.0f}, .max{200.0f}, .on_change = applyDistance});
    cvars.add<float>(
        {.name = "nameplatePlacement", .init = 0.0f, .min{-1.0f}, .max{2.0f}, .fmt = "%.4f", .on_change = markDirty});
    cvars.add<int>({.name = "nameplateMouseMode", .init = 0, .min{0}, .max{kMouseModeMax}, .on_change = applyDerived});
    cvars.add<float>({.name = "nameplateBandX", .init = 0.7f, .min{0.1f}, .max{1.0f}, .on_change = markDirty});
    cvars.add<float>({.name = "nameplateBandY", .init = 1.0f, .min{0.1f}, .max{1.5f}, .on_change = markDirty});
    cvars.add<int>(
        {.name = "nameplateHitboxAnchor", .init = 1, .min{kHitboxTop}, .max{kHitboxBottom}, .on_change = markDirty});
    cvars.add<float>({.name = "nameplateHitboxWidthE", .init = 1.0f, .min{0.0f}, .max{1.0f}, .on_change = markDirty});
    cvars.add<float>({.name = "nameplateHitboxHeightE", .init = 1.0f, .min{0.0f}, .max{1.0f}, .on_change = markDirty});
    cvars.add<float>({.name = "nameplateHitboxWidthF", .init = 1.0f, .min{0.0f}, .max{1.0f}, .on_change = markDirty});
    cvars.add<float>({.name = "nameplateHitboxHeightF", .init = 1.0f, .min{0.0f}, .max{1.0f}, .on_change = markDirty});
    cvars.add<float>(
        {.name = "nameplateRaiseSpeed", .init = 100.0f, .min{1.0f}, .max{250.0f}, .on_change = applyDerived});
    cvars.add<float>(
        {.name = "nameplateLowerSpeed", .init = 100.0f, .min{1.0f}, .max{250.0f}, .on_change = applyDerived});
    cvars.add<float>(
        {.name = "nameplatePullSpeed", .init = 50.0f, .min{1.0f}, .max{250.0f}, .on_change = applyDerived});
    cvars.add<float>({.name = "nameplateRaiseDistance", .init = 8.0f, .min{1.0f}, .max{20.0f}, .on_change = markDirty});
    cvars.add<float>({.name = "nameplatePullDistance", .init = 0.25f, .min{0.0f}, .max{0.75f}, .on_change = markDirty});
    cvars.add<float>(
        {.name = "nameplateOcclusionAlpha", .init = 1.0f, .min{-1.0f}, .max{1.0f}, .on_change = applyDerived});
    cvars.add<int>({.name = "nameplateOcclusionMode", .init = 0, .min{0}, .max{1}, .on_change = markDirty});
    cvars.add<float>(
        {.name = "nameplateNonTargetAlpha", .init = 0.5f, .min{0.0f}, .max{1.0f}, .on_change = applyDerived});
    cvars.add<float>(
        {.name = "nameplateAlphaSpeed", .init = 0.25f, .min{0.01f}, .max{1.0f}, .fmt = "%.3f", .on_change = markDirty});
    cvars.add<float>({.name = "nameplateInertia", .init = 1.0f, .min{0.0f}, .max{20.0f}, .on_change = markDirty});
    cvars.add<float>(
        {.name = "nameplateHysteresisDecay", .init = 1.0f, .min{0.25f}, .max{30.0f}, .on_change = markDirty});
    cvars.add<int>({.name = "nameplateClampMode", .init = 0, .min{0}, .max{kClampMax}, .on_change = applyClampMode});
    cvars.add<float>(
        {.name = "nameplateClampModeVOffset", .init = 0.01f, .min{0.0f}, .max{0.05f}, .on_change = markDirty});
    cvars.add<float>(
        {.name = "nameplateClampModeHOffset", .init = 0.01f, .min{0.0f}, .max{0.1f}, .on_change = markDirty});
    cvars.add<int>(
        {.name = "nameplateClampModeFilter", .init = 0, .min{0}, .max{kClampFilterMax}, .on_change = markDirty});
    cvars.add<int>({.name = "nameplateMouseFreeze", .init = 0, .min{0}, .max{kFreezeMax}, .on_change = markDirty});
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

    tx.attach(CGWorldFrame::updateNamePlatePositions_hook{}, worldRenderDirty_site{}, CGUnit_C::isVisible_hook{},
        CGUnit_C::updateReaction_hook{}, CGUnit_C::setNamePlateFocus_hook{}, CGUIBindings::setBindingModeState_hook{},
        game_ui::target_hook{}, game_ui::clearTarget_hook{}, CGNamePlateFrame::ctor_hook{},
        CGNamePlateFrame::wipeActive_hook{}, CGNamePlateFrame::wipeCDA_hook{}, plateLevel_site{}, plateAlpha_site{});
}
