#pragma once

#include <cstdint>

class CGInputControl {
public:
    enum ControlFlag {
        // the bits follow the binding action, not the physical button
        eTurnOrAction = 0x1,
        eCameraOrSelect = 0x2,
        eMoveForward = 0x10,
        eMoveBackward = 0x20,
        eStrafeLeft = 0x40,
        eStrafeRight = 0x80,
        eTurnLeft = 0x100,
        eTurnRight = 0x200,
        eAutoRun = 0x1000,
        eIsMoving = 0x10000,
        eIsStrafing = 0x20000,
        eIsTurning = 0x40000,
        eIsPitching = 0x80000,
        eCamera200000 = 0x200000,
        eCamera400000 = 0x400000,
        eCamera800000 = 0x800000,
        eCamera1000000 = 0x1000000,
        eMouseLook = 0x2000000,
        eMouseSteer = 0x4000000,
    };

    enum MouseMode {
        eMouseModeHidden = 0x1,
        eMouseMode2 = 0x2,
        eMouseMode4 = 0x4,
        eMouseMode6 = 0x6,
        eMouseMode10 = 0x10,
        eMouseMode20 = 0x20,
        eMouseMode40 = 0x40,
    };

    enum ClickType {
        eClickNone = 0x0,
        eClickLeft = 0x1,
        eClickRight = 0x2,
    };

    struct MOUSELOOKBINDING {
        TSHashObject<MOUSELOOKBINDING> hash_obj;
        char* action;
    };

    static constexpr uint32_t kMouseBusyFlags = CGInputControl::eTurnOrAction | CGInputControl::eCameraOrSelect |
        CGInputControl::eMouseLook | CGInputControl::eMouseSteer;

    uint32_t world_enter_time_;
    uint32_t control_flags_;
    float mouse_x_;
    float mouse_y_;
    uint32_t field_10_;
    uint32_t mouse_press_time_;  // input event clock, set when the first mouse bit goes down
    uint32_t click_type_;
    TSHashTable<MOUSELOOKBINDING> bindings_table_;
    uint32_t has_override_yaw_;
    float override_yaw_;
    uint32_t has_override_pitch_;
    float override_pitch_;
    uint32_t both_mouse_buttons_pressed_;
    uint32_t mouse_mode_state_;
    uint32_t joystick_enabled_;
    uint32_t joystick_state_;
    float mouse_delta_x_;
    float mouse_delta_y_;
    void* wow_mouse_handle_;

    // mirrors IsMouseDrag (0x005F9600)
    [[nodiscard]]
    bool isMouseDrag(int32_t held_ms) const {
        if (held_ms >= 800) { return true; }
        return (mouse_x_ >= 8.0f || mouse_y_ >= 8.0f) && held_ms >= 200;
    }

    HOOKKIT_HOOK_HANDLE(get, 0x005F95D0, hookkit::Conv::eCdecl, CGInputControl*);
};

static_assert(sizeof(CGInputControl) == 0x70);
