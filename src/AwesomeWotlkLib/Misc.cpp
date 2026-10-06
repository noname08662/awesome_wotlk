#include "Misc.h"

#include <corecrt_math_defines.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Extensions.h"
#include "MiscCaptureFrame.h"
#include "MiscCaptureFrameLua.h"
#include "Utils.h"

#include "Spell/CSpell_C.h"
#include "include/DB/DBRecords.h"
#include "include/FrameScript/FrameScript.h"
#include "include/Game/CGGameUI.h"
#include "include/Graphics/CGxDevice.h"
#include "include/Graphics/CGxDeviceD3d.h"
#include "include/Lib/Lua.h"
#include "include/M2/CM2Model.h"
#include "include/Math/Primitives.h"
#include "include/ObjectManager/CGGameObject_C.h"
#include "include/ObjectManager/CGPlayer_C.h"
#include "include/ObjectManager/DescriptorsEnums.h"
#include "include/ObjectManager/ObjectManager.h"
#include "include/ObjectManager/ObjectManagerEnums.h"
#include "include/Spell/CSpell_C.h"
#include "include/System/System.h"
#include "include/Texture/CTexture.h"
#include "include/UI/UIBindings.h"
#include "include/Widget/CSimpleFrame.h"
#include "include/World/CGWorldFrame.h"

namespace {
constexpr DWORD kCandidateScanIntervalMs = 100;

constexpr std::array<uint8_t, 12> kValidGameobjectTypes = {
    eGameobjectTypeDoor,
    eGameobjectTypeButton,
    eGameobjectTypeQuestgiver,
    eGameobjectTypeChest,
    eGameobjectTypeBinder,
    eGameobjectTypeChair,
    eGameobjectTypeSpellFocus,
    eGameobjectTypeGoober,
    eGameobjectTypeFishingnode,
    eGameobjectTypeMailbox,
    eGameobjectTypeMeetingstone,
    eGameobjectTypeGuildBank,
};

struct InteractState {
    guid_t request = 0;      // one-shot interaction queued from Lua
    guid_t candidate = 0;    // nearest valid interact target
    guid_t highlighted = 0;  // candidate currently showing the interact highlight (in range, cvar on)
} interact;

bool isInteractableGameObject(uint8_t type) {
    return std::ranges::any_of(kValidGameobjectTypes, [type](uint8_t t) { return t == type; });
}

bool isValidObject(CGObject_C* object, CGUnit_C* player, float distance_sq) {
    if (object->type_id_ == eTypeidUnit) {
        uint32_t dyn_flags = object->getValue<uint32_t>(eUnitDynamicFlags);
        uint32_t unit_flags = object->getValue<uint32_t>(eUnitFieldFlags);
        uint32_t npc_flags = object->getValue<uint32_t>(eUnitNpcFlags);

        bool is_lootable = (dyn_flags & eUnitDynflagLootable) != 0;
        bool is_skinnable = (unit_flags & eUnitFlagSkinnable) != 0;
        bool can_assist = player->canAssist(reinterpret_cast<CGUnit_C*>(object), true);

        bool flags_ok = is_lootable || is_skinnable || (can_assist && npc_flags != 0);
        return flags_ok && distance_sq <= reinterpret_cast<CGUnit_C*>(object)->getInteractDistanceSq();
    }
    if (object->type_id_ == eTypeidGameobject) {
        auto* go = object->as<CGGameObject_C>();
        return go != nullptr && isInteractableGameObject(go->descriptors_->subtype) && go->canUse() && go->canUseNow();
    }
    return false;
}

void processQueuedInteraction() {
    if (interact.request == 0) { return; }
    if (auto* object =
            object_mgr::get<CGObject_C>(interact.request, static_cast<TypeMask>(eTypemaskGameobject | eTypemaskUnit))) {
        object->onRightClick();
    }
    interact.request = 0;
}

void updateInteractCandidate() {
    if (!game_ui::isInWorld()) { return; }

    static DWORD last_scan_tick = 0;
    DWORD now = GetTickCount();
    if (now - last_scan_tick < kCandidateScanIntervalMs) { return; }
    last_scan_tick = now;

    guid_t candidate = 0;
    float best_distance_sq = 400.0f;  // 20.0f squared

    auto* player = object_mgr::get<CGPlayer_C>(object_mgr::getPlayerGuid(), eTypemaskPlayer);
    if (player == nullptr) { return; }

    auto angle_degrees = static_cast<float>(extensions::console::kCvarRegistry->ref<"interactionAngle", int>()) * 0.5f;
    bool look_in_angle = extensions::console::kCvarRegistry->ref<"interactionMode", int>() == 1;

    float facing = player->getFacing();
    Vec3f pos_player{};
    player->getPosition(pos_player);

    auto try_set_candidate = [&](guid_t guid) {
        auto* object = object_mgr::get<CGObject_C>(guid, static_cast<TypeMask>(eTypemaskGameobject | eTypemaskUnit));
        if (object == nullptr) { return; }

        auto distance_sq = static_cast<float>(object->getDistanceToPosSq(&pos_player));
        if (distance_sq == 0.0f || distance_sq > 400.0f || distance_sq > best_distance_sq) { return; }

        if (!isValidObject(object, player, distance_sq)) { return; }

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

        candidate = guid;
        best_distance_sq = distance_sq;
    };

    object_mgr::enumObjects([&](guid_t guid) {
        if (guid != player->getGuid()) { try_set_candidate(guid); }
        return true;
    });

    if (interact.candidate != candidate) { interact.candidate = candidate; }

    guid_t visual_candidate =
        (extensions::console::kCvarRegistry->ref<"interactionHighlight", int>() != 0) ? candidate : 0;

    if (interact.highlighted != visual_candidate) {
        if (interact.highlighted != 0) {
            if (auto* old_obj = object_mgr::get<CGObject_C>(
                    interact.highlighted, static_cast<TypeMask>(eTypemaskGameobject | eTypemaskUnit))) {
                old_obj->hideHighlightType(CGObject_C::eHighlightTypeInteract);
            }
        }
        interact.highlighted = visual_candidate;
        if (interact.highlighted != 0) {
            if (auto* new_obj = object_mgr::get<CGObject_C>(
                    interact.highlighted, static_cast<TypeMask>(eTypemaskGameobject | eTypemaskUnit))) {
                new_obj->showHighlightType(CGObject_C::eHighlightTypeInteract);
                if ((new_obj->highlight_mask_ & CGObject_C::eHighlightMaskNative) == 0) {
                    if (CM2Model* model = new_obj->getObjectModel()) { model->emissive_color_ *= 0.8f; }
                }
            }
        }
    }
}

int luaQueueInteract(LuaState* l) {
    if (!game_ui::isInWorld()) { return 0; }
    if (!lua::isNoneOrNil(l, 1)) {
        const char* raw = lua::toString(l, 1);
        if (raw == nullptr) { return 0; }
        std::string mod_str = raw;

        for (char c : mod_str) {
            if (std::isalnum(static_cast<unsigned char>(c)) == 0) { return 0; }
        }
        if (guid_t guid = object_mgr::guidFromUnitId{}(mod_str.c_str())) { interact.request = guid; }
    } else if (interact.candidate != 0) {
        interact.request = interact.candidate;
    }
    return 0;
}

int interactFunctionC(LuaState* l) {
    const char* param = nullptr;
    if (!lua::isNoneOrNil(l, 1)) { param = lua::toString(l, 1); }

    lua::pushCFunction(l, reinterpret_cast<lua::LuaCFunction>(framescript::secureCmdOptionsParse::kAddress));

    if (param != nullptr) {
        lua::pushString(l, param);
    } else {
        lua::pushNil(l);
    }

    if (lua::pcall(l, 1, 2, 0) != 0) {
        lua::pop(l, 1);
        lua::pushCFunction(l, luaQueueInteract);
        if (lua::isFunction(l, -1)) {
            if (lua::pcall(l, 0, 0, 0) != 0) { lua::pop(l, 1); }
        }
        return 0;
    }

    if (!lua::isNil(l, -1)) {
        lua::pushCFunction(l, luaQueueInteract);
        if (lua::isFunction(l, -1)) {
            lua::pushValue(l, -2);
            if (lua::pcall(l, 1, 0, 0) != 0) { lua::pop(l, 1); }
        }
        lua::pop(l, 3);
    } else {
        lua::pop(l, 1);
        lua::pushCFunction(l, luaQueueInteract);
        if (lua::isFunction(l, -1)) {
            if (lua::pcall(l, 0, 0, 0) != 0) { lua::pop(l, 1); }
        }
        lua::pop(l, 1);
    }
    return 0;
}
}  // namespace

namespace {
enum ObjHlMode : int {
    eHlDisabled,
    eHlAlways,
    eHlTracked,
};

enum PendingClick : int { eNone, eCursor, ePlayerLocation, eBlocked };

auto pending_click = PendingClick::eNone;

HOOKKIT_BIND(framescript::secureCmdOptionsParse_hook, [](LuaState* l) {
    int result = framescript::secureCmdOptionsParse{}(l);
    if (lua::getTop(l) < 3 || !lua::isString(l, 2) || !lua::isString(l, 3)) { return result; }

    if (!CSpell_C::isTargetingAoE{}()) { pending_click = PendingClick::eNone; }
    if (pending_click == PendingClick::eBlocked) { return result; }

    std::string_view parsed_target_view = lua::toString(l, 3);
    bool is_cursor = utils::iequals(parsed_target_view, "cursor");
    bool is_playerlocation = utils::iequals(parsed_target_view, "playerlocation");

    if (!is_cursor && !is_playerlocation) { return result; }

    pending_click = is_cursor ? PendingClick::eCursor : PendingClick::ePlayerLocation;

    std::string parsed_result = lua::toString(l, 2);
    std::string orig_string = lua::toString(l, 1);

    lua::pop(l, 3);
    lua::pushString(l, orig_string.c_str());
    lua::pushString(l, parsed_result.c_str());
    lua::pushNil(l);

    return result;
});

HOOKKIT_BIND(CGWorldFrame::onLayerTrackTerrain_hook, [](CGWorldFrame* self, CGWorldFrame::TerrainClickEvent* click) {
    if (CSpell_C::isTargetingAoE{}()) {
        if (pending_click == PendingClick::eNone) {
            pending_click = PendingClick::eBlocked;
            return self->onLayerTrackTerrain(click);
        }
        if (pending_click == PendingClick::eBlocked) { return self->onLayerTrackTerrain(click); }
    }

    auto* player = object_mgr::get<CGPlayer_C>(object_mgr::getPlayerGuid(), eTypemaskPlayer);
    if (player == nullptr) { return self->onLayerTrackTerrain(click); }

    PendingClick pending = std::exchange(pending_click, PendingClick::eNone);
    if (pending == PendingClick::ePlayerLocation) {
        Vec3f player_pos{};
        player->getPosition(player_pos);

        CGWorldFrame::TerrainClickEvent tc{.guid = 0, .pos = Vec3f{player_pos}, .button = 1};
        CGWorldFrame::handleTerrainClick{}(&tc);
    } else if (pending == PendingClick::eCursor) {
        CGWorldFrame::TerrainClickEvent tc{.guid = 0, .pos = Vec3f{click->pos}, .button = 1};
        CGWorldFrame::handleTerrainClick{}(&tc);
    }
    return self->onLayerTrackTerrain(click);
});

HOOKKIT_BIND(CSpell_C::cancelPendingAoeTargeting_hook, []() {
    pending_click = PendingClick::eNone;
    return CSpell_C::cancelPendingAoeTargeting{}();
});

HOOKKIT_BIND(CGxDevice::projectTex2d_hook,
    [](AaBox* bbox, Vec4u8* color, Mat4f* matrix, float z_bias, int flags, char vtx_mode, float depth_bias) {
        AaBox new_bbox = *bbox;

        // expand Z projection range to prevent clipping on steep terrain
        float center_z = (new_bbox.max.z + new_bbox.min.z) * 0.5f;
        new_bbox.min.z = center_z - 50.0f;
        new_bbox.max.z = center_z + 50.0f;

        return CGxDevice::projectTex2d{}(&new_bbox, color, matrix, z_bias, flags, vtx_mode, depth_bias);
    });

// attached only while interactionHighlight is on
HOOKKIT_BIND(CGObject_C::hideHighlightType_hook, [](CGObject_C* self, CGObject_C::HighlightType highlight_type) {
    int result = self->hideHighlightType(highlight_type);
    if (highlight_type != CGObject_C::eHighlightTypeInteract && interact.highlighted != 0 &&
        self->getGuid() == interact.highlighted) {
        if ((self->highlight_mask_ & CGObject_C::eHighlightMaskNative) == 0) {
            if (CM2Model* model = self->getObjectModel()) { model->emissive_color_ *= 0.8f; }
        }
    }
    return result;
});

void applyInteractionHighlight(int enabled) {
    static_cast<void>(hookkit::HookTransaction::reinstall(CGObject_C::hideHighlightType_hook{},
        enabled != 0 ? CGObject_C::hideHighlightType_hook::resolveDetour() : nullptr));
}

// one detour per objectHighlightMode other than eHlDisabled, which runs the engine's
template <ObjHlMode Mode>
inline constexpr auto kPassiveHighlight = [](CGGameObject_C* self) {
    self->checkForPassiveHighlight();

    GameobjectTypes go_type = self->descriptors_->subtype;
    if (!self->canUse() ||
        (go_type != eGameobjectTypeChest && go_type != eGameobjectTypeGoober && go_type != eGameobjectTypeQuestgiver)) {
        return;
    }
    if constexpr (Mode == ObjHlMode::eHlTracked) {
        if (go_type == eGameobjectTypeQuestgiver && self->quest_model_ == nullptr) { return; }
        if (go_type == eGameobjectTypeChest) {
            if (const LockRec* lock_rec = self->getLockRec()) {
                if (lock_rec->type[0] == LockRec::eLockKeySkill) { return; }  // gathering node
            }
        }
    }
    self->highlight_mask_ |= CGObject_C::eHighlightMaskPassiveLootGlow;
    self->showLootEffect();
};
}  // namespace

namespace {
constexpr int kEnginePortraitRes = 64;
int portrait_res = kEnginePortraitRes;

void applyPortraitRes(int res) {
    portrait_res = static_cast<int>(std::bit_ceil(static_cast<unsigned int>(res)));

    CTexture*& depth = game_ui::portrait_depth_texture;
    if (depth == nullptr || std::cmp_less_equal(portrait_res, depth->width_)) { return; }
    const auto size = static_cast<uint32_t>(portrait_res);
    // the engine's flags as AllocAndRegister stored them; it re-derives filter and anisotropy the same way again
    const uint32_t flags = depth->creation_flags_.packed;
    if (CTexture* grown = CTexture::allocAndRegister{}(
            0, size, size, 0x18, 12, 12, flags, nullptr, reinterpret_cast<void*>(0x005EEB70), "PortraitDepth", 1)) {
        CHandle::close{}(reinterpret_cast<CHandle*>(std::exchange(depth, grown)));
    }
}

HOOKKIT_NAMED_BIND_RAW(portraitInitialize_site, 0x006180E0, {"jmpback", 0x006180E5}) {
    constexpr uintptr_t kJmpback = portraitInitialize_site::target("jmpback");
    const auto res = reinterpret_cast<uintptr_t>(&portrait_res);
    return CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(
        jmpTo(kJmpback, movRegAbs(Reg::eDx, res), pushReg(Reg::eDx), pushReg(Reg::eDx), pushReg(Reg::eSi)));
};

HOOKKIT_NAMED_BIND_RAW(portraitRender_site, 0x00619B6A, {"jmpback", 0x00619B72}) {
    constexpr uintptr_t kJmpback = portraitRender_site::target("jmpback");
    const uintptr_t size = HOOKKIT_LAMBDA_ADDR([] {
        if (game_ui::portrait_use_render_target == 0) { return kEnginePortraitRes; }
        const CTexture* depth = game_ui::portrait_depth_texture;
        return depth != nullptr ? std::min<int>(portrait_res, depth->width_) : portrait_res;
    });
    return CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(
        size, jmpTo(kJmpback, pushImm(2), pushImm(2), pushReg(Reg::eAx), pushReg(Reg::eAx)), 0);
};
}  // namespace

namespace {
void applyChatLogStamp(int enabled) {
    static std::array<char, MAX_PATH> chat_log_path{};

    const char* path;
    if (enabled != 0) {
        auto [ptr, count] = std::format_to_n(
            chat_log_path.data(), chat_log_path.size() - 1, "Logs\\{}WoWChatLog.txt", utils::sessionStamp());
        *ptr = '\0';
        path = chat_log_path.data();
    } else {
        path = reinterpret_cast<const char*>(0x009FA4E4);
    }

    hookkit::forceWrite<const char*>(0x00AC7A40, path);
}

void applyCombatLogStamp(int enabled) {
    static std::array<char, MAX_PATH> combat_log_path{};

    const char* path;
    if (enabled != 0) {
        auto [ptr, count] = std::format_to_n(
            combat_log_path.data(), combat_log_path.size() - 1, "Logs\\{}WoWCombatLog.txt", utils::sessionStamp());
        *ptr = '\0';
        path = combat_log_path.data();
    } else {
        path = reinterpret_cast<const char*>(0x009FA4CC);
    }

    hookkit::forceWrite<const char*>(0x00AC7A44, path);
}

void applyObjHlMode(int mode, bool changed) {
    auto new_mode = static_cast<ObjHlMode>(mode);

    using Hook = CGGameObject_C::checkForPassiveHighlight_hook;
    void* detour = nullptr;
    if (new_mode == eHlAlways) {
        detour = Hook::staticDetour<kPassiveHighlight<eHlAlways>>();
    } else if (new_mode == eHlTracked) {
        detour = Hook::staticDetour<kPassiveHighlight<eHlTracked>>();
    }
    const bool switched = Hook::attached.load() ? Hook::resolveDetour() != detour : detour != nullptr;
    if (switched && hookkit::HookTransaction::reinstall(Hook{}, detour) != NO_ERROR) { return; }
    if (!changed && !switched) { return; }

    if (guid_t player_guid = object_mgr::getPlayerGuid();
        object_mgr::get<CGPlayer_C>(player_guid, eTypemaskPlayer) != nullptr) {
        object_mgr::enumObjects([&](guid_t guid) {
            if (guid < 0x1000) { return true; }

            auto* go = object_mgr::get<CGGameObject_C>(guid, eTypemaskGameobject);
            if (go == nullptr || go->type_id_ != eTypeidGameobject) { return true; }

            if (new_mode != eHlDisabled) {
                CGGameObject_C::checkForPassiveHighlight_hook::viaDetour(go);
            } else {
                go->checkForPassiveHighlight();
            }
            return true;
        });
    }
}
}  // namespace

namespace {
int luaFlashWindow(LuaState*) {
    if (HWND hwnd = os::getGameWindow()) { FlashWindow(hwnd, FALSE); }
    return 0;
}

int luaIsWindowFocused(LuaState* l) {
    HWND hwnd = os::getGameWindow();
    if (hwnd == nullptr || GetForegroundWindow() != hwnd) { return 0; }
    lua::pushNumber(l, 1.0);
    return 1;
}

int luaFocusWindow(LuaState*) {
    if (HWND hwnd = os::getGameWindow()) { SetForegroundWindow(hwnd); }
    return 0;
}

int luaCopyToClipboard(LuaState* l) {
    const char* str = lua::checkString(l, 1);
    if (str != nullptr) {
        const std::string_view text{str};
        if (!text.empty()) { utils::copyToClipboardU8(str, nullptr); }
    }
    return 0;
}

int luaCaptureFrame(LuaState* l) {
    CSimpleFrame* frame = lua::toFrame(l);
    if (frame == nullptr) { return 0; }
    if (frame == CGWorldFrame::get()) {
        misc_capture_frame::frame_capture.captureFailed("FRAMECAPTURE_WORLD_FRAME");
        return 0;
    }
    misc_capture_frame::CaptureRequest request{.frame = frame};
    if (lua::isNumber(l, 2)) {
        request.longest_side = static_cast<float>(lua::toNumber(l, 2));
    } else if (lua::isString(l, 2)) {
        const std::string_view text = lua::toString(l, 2);
        const bool is_scale = !text.empty() && (text.back() == 'x' || text.back() == 'X');
        const std::string_view number = is_scale ? text.substr(0, text.size() - 1) : text;
        const char* const last = std::to_address(number.end());
        double value = 0.0;
        const auto [ptr, ec] = std::from_chars(number.data(), last, value);
        if (ec != std::errc{} || ptr != last || !(value > 0.0)) {
            misc_capture_frame::frame_capture.captureFailed(
                "FRAMECAPTURE_INVALID_SIZE", {misc_capture_frame::fmt::fromString(text)});
            return 0;
        }
        (is_scale ? request.scale : request.longest_side) = static_cast<float>(value);
    }
    misc_capture_frame::frame_capture.pending = request;
    misc_capture_frame::updateCaptureHooks();
    return 0;
}

int luaOpenMisc(LuaState* l) {
    static constexpr std::array<lua::LuaLReg, 6> kFuncs = {{
        {.name = "FlashWindow", .func = luaFlashWindow},
        {.name = "IsWindowFocused", .func = luaIsWindowFocused},
        {.name = "FocusWindow", .func = luaFocusWindow},
        {.name = "CopyToClipboard", .func = luaCopyToClipboard},
        {.name = "QueueInteract", .func = luaQueueInteract},
        {.name = "CaptureFrame", .func = luaCaptureFrame},
    }};
    for (const auto& [name, func] : kFuncs) {
        lua::pushCFunction(l, func);
        lua::setGlobal(l, name);
    }
    return 0;
}

void onEnterWorld() {
    lua::registerSlashCommand("INTERACTCMD", "/interact", interactFunctionC);
    CGUIBindings::get()->registerBinding("AWESOME_KEYBIND", "INTERACTIONKEYBIND", "Interaction Button",
        "AWESOME_WOTLK_KEYBINDS", "Awesome Wotlk Keybinds", "QueueInteract()");
    static_cast<void>(framescript::execute{}(misc_capture_frame_lua::kFrameCaptureCommands, "FrameCapture", nullptr));

    if (LuaState* l = lua::getLuaState()) {
        const char* chat_path = *reinterpret_cast<const char* const*>(0x00AC7A40);
        std::string chat_msg = std::string("Chat being logged to ") + ((chat_path != nullptr) ? chat_path : "");
        lua::pushString(l, chat_msg.c_str());
        lua::setGlobal(l, "CHATLOGENABLED");

        const char* combat_path = *reinterpret_cast<const char* const*>(0x00AC7A44);
        std::string combat_msg = std::string("Combat being logged to ") + ((combat_path != nullptr) ? combat_path : "");
        lua::pushString(l, combat_msg.c_str());
        lua::setGlobal(l, "COMBATLOGENABLED");
    }
}

void onLeaveWorld() {
    interact = {};
    misc_capture_frame::frame_capture.pending = {};
    misc_capture_frame::updateCaptureHooks();
}

void onUpdate() {
    updateInteractCandidate();
    processQueuedInteraction();
}
}  // namespace

void misc::initialize(hookkit::HookTransaction& tx) {
    auto& cvars = *extensions::console::kCvarRegistry;

    cvars.add<int>({.name = "interactionAngle", .init = 60, .min{15}, .max{160}});
    cvars.add<int>({.name = "interactionMode", .init = 1, .min{0}, .max{1}});
    cvars.add<int>(
        {.name = "interactionHighlight", .init = 1, .min{0}, .max{1}, .on_change = applyInteractionHighlight});
    cvars.add<int>({.name = "portraitResolution", .init = 64, .min{64}, .max{2048}, .on_change = applyPortraitRes});
    cvars.add<int>({.name = "objectHighlightMode", .init = 0, .min{0}, .max{2}, .on_change = applyObjHlMode});
    cvars.add<int>({.name = "chatLogSessionKey", .init = 1, .min{0}, .max{1}, .on_change = applyChatLogStamp});
    cvars.add<int>({.name = "combatLogSessionKey", .init = 1, .min{0}, .max{1}, .on_change = applyCombatLogStamp});

    extensions::console::kLuaLibRegistry->add(luaOpenMisc);

    extensions::framescript::kOnEnter->add(onEnterWorld);
    extensions::framescript::kOnLeave->add(onLeaveWorld);
    extensions::framescript::kOnUpdate->add(onUpdate);

    tx.attach(framescript::secureCmdOptionsParse_hook{}, CGWorldFrame::onLayerTrackTerrain_hook{},
        CSpell_C::cancelPendingAoeTargeting_hook{}, CGxDevice::projectTex2d_hook{}, portraitInitialize_site{},
        portraitRender_site{});
}
