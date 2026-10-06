#pragma once

#include <cstdint>

#include "BaseTypes.h"

#include "Math/Primitives.h"
#include "Widget/CSimpleCamera.h"

class CM2Model;

class CGCamera : public CSimpleCamera {
public:
    enum CamFlags3 : uint32_t {
        eCam3FollowFacing = 0x00000001,
        eCam3ModelCamera = 0x00000004,
        eCam3ViewLocked = 0x00000008,
        eCam3HasTarget = 0x00000010,
        eCam3DisableInput = 0x00000020,
        eCam3MouseHeld = 0x00000040,
        eCam3ClickToMove = 0x00000100,
        eCam3FreelookActive = 0x00000200,
        eCam3ClickToMovE2 = 0x00000400,
        eCam3InheritYaw = 0x00001000,
        eCam3DisableFollow = 0x00004000,
        eCam3DisableSmoothing = 0x00008000,
        eCam3IsMoving = 0x00010000,
        eCam3CanPivot = 0x00020000,
        eCam3CameraBelow = 0x00100000,
        eCam3CameraAbove = 0x00200000,
        eCam3FovAltBlend = 0x00400000,
        eCam3TargetChanged = 0x00800000,
        eCam3TransitionT2 = 0x01000000,
        eCam3PitchLimitBlend = 0x02000000,
        eCam3IsColliding = 0x04000000,
        eCam3PitchSmoothActive = 0x08000000,
        eCam3SmoothOverride = 0x10000000,
        eCam3YawBlendActive = 0x20000000,
        eCam3FovBlendActive = 0x40000000,
        eCam3TelescopeActive = 0x80000000,
    };

    enum CamMode : uint32_t {
        eCamModeAutoInteract = 0x00000001,
        eCamModeHasTarget = 0x00000002,
        eCamModeFacingLocked = 0x00000004,
        eCamModeAltZoom = 0x00000020,
        eCamModeZoomLocked = 0x00000040,
    };

    enum ZoomState : uint32_t {
        eCamzoomZoomInActive = 0x00000001,
        eCamzoomZoomInInterrupted = 0x00000002,
        eCamzoomZoomOutActive = 0x00000004,
        eCamzoomZoomOutInterrupted = 0x00000008,
    };

    struct ViewEntry {
        float pitch_limit;
        float zoom_dist;
        float facing;
    };

    struct CameraShake {
        TSLink<CameraShake> link;
        int type;
        int channel;  // axis index 0-2
        float amplitude;
        float param1;
        float duration;  // lifetime
        float phase_offset;
        float param4;
        Vec3f dir;
        uint32_t start_time_ms;
    };

    static_assert(sizeof(CameraShake) == 0x34);

    CM2Model* model_;         // 0x48
    uint32_t timestamp_;      // 0x4C
    uint32_t model_handle_;   // 0x50
    Mat3f model_cam_matrix_;  // 0x54
    Vec3f target_pos_;        // 0x78
    float _unk_84;            // 0x84
    guid_t target_guid_;      // 0x88

    uint32_t flags_;          // 0x90
    uint32_t flags2_;         // 0x94
    CamFlags3 flags3_;        // 0x98
    CamMode cam_mode_flags_;  // 0x9C

    guid_t relative_to_guid_;         // 0xA0
    float terrain_tilt_angle_;        // 0xA8
    uint32_t target_retain_count_;    // 0xAC
    uint32_t current_view_index_;     // 0xB0
    uint32_t current_view_slot_;      // 0xB4
    float current_view_pitch_limit_;  // 0xB8
    float current_view_facing_;       // 0xBC
    ViewEntry view_table_[6];         // 0xC0
    float commentator_yaw_;           // 0x108
    Vec3f model_cam_offset_;          // 0x10C - {zoom, pitch, 0}

    float zoom_distance_;             // 0x118
    float facing_smooth_;             // 0x11C - animated
    float pitch_limit_max_;           // 0x120 - animated, collision-clamped
    float pitch_limit_min_;           // 0x124
    float yaw_;                       // 0x128 - animated
    float pivot_accum_;               // 0x12C
    float pitch_offset_;              // 0x130 - (actual pitch - pitch_limit_max_)
    float saved_first_person_pitch_;  // 0x134
    float fov_correction_;            // 0x138
    float fov_correction_target_;     // 0x13C
    Vec3f shake_bias_;                // 0x140
    float pitch_limits_[3];           // 0x14C - per-axis height bounds

    unk_t _unk_158[2];                  // 0x158
    ZoomState zoom_state_flags_;        // 0x160
    uint32_t zoom_start_in_out_ms_[2];  // 0x164
    unk_t _unk_16C;                     // 0x16C
    unk_t _unk_170[3];                  // 0x170
    uint32_t zoom_out_start_ms_;        // 0x17C
    uint32_t zoom_in_start_ms_;         // 0x180
    unk_t _unk_184;                     // 0x184
    unk_t _unk_188[2];                  // 0x188
    unk_t _unk_190;                     // 0x190
    uint32_t zoom_in_end_;              // 0x194
    uint32_t zoom_out_end_;             // 0x198
    unk_t _unk_19C[3];                  // 0x19C
    float pending_zoom_out_delta_;      // 0x1A8
    float zoom_speed_[2];               // 0x1AC
    unk_t _unk_1B4[4];                  // 0x1B4

    float cached_target_x_;                 // 0x1C4
    float cached_target_y_;                 // 0x1C8
    float cached_target_z_;                 // 0x1CC
    float cached_something_;                // 0x1D0
    float cached_facing_;                   // 0x1D4
    uint32_t terrain_tilt_last_update_ms_;  // 0x1D8

    float saved_facing_;  // 0x1DC - freelook start

    uint32_t zoom_blend_start_ms_;  // 0x1E0
    float zoom_blend_duration_;     // 0x1E4
    float zoom_blend_from_;         // 0x1E8 - source
    float zoom_blend_to_;           // 0x1EC - destination
    float zoom_limit_blend_end_;    // 0x1F0
    float zoom_limit_blend_start_;  // 0x1F4

    uint32_t smooth_override_start_ms_;  // 0x1F8
    float smooth_override_duration_;     // 0x1FC
    float smooth_override_from_;         // 0x200
    float smooth_override_to_;           // 0x204
    float smooth_override_end_;          // 0x208
    float smooth_override_time_;         // 0x20C

    uint32_t yaw_blend_start_ms_;  // 0x210
    float yaw_blend_duration_;     // 0x214
    float yaw_blend_from_;         // 0x218
    float yaw_blend_to_;           // 0x21C
    float yaw_blend_end_;          // 0x220 - 1.0 default
    float yaw_blend_start_;        // 0x224

    uint32_t pitch_limit_blend_start_ms_;  // 0x228
    float pitch_limit_blend_duration_;     // 0x22C
    float pitch_limit_blend_from_;         // 0x230
    float pitch_limit_blend_to_;           // 0x234
    float pitch_limit_blend_start_;        // 0x238
    float pitch_limit_blend_time_;         // 0x23C

    uint32_t pitch_blend_start_ms_;  // 0x240
    float pitch_blend_duration_;     // 0x244
    float pitch_blend_from_;         // 0x248
    float pitch_blend_to_;           // 0x24C
    float pitch_blend_end_;          // 0x250
    float pitch_blend_start_;        // 0x254

    uint32_t facing_limit_blend_start_ms_;  // 0x258
    float facing_limit_blend_duration_;     // 0x25C
    float facing_limit_from_;               // 0x260
    float facing_limit_to_;                 // 0x264
    float facing_limit_blend_end_;          // 0x268
    float facing_limit_blend_start_;        // 0x26C

    uint32_t fov_blend_start_ms_;  // 0x270
    float fov_blend_duration_;     // 0x274
    float fov_blend_from_;         // 0x278
    float fov_blend_to_;           // 0x27C
    float fov_blend_end_;          // 0x280
    float fov_blend_start_;        // 0x284

    uint32_t freelook_start_ms_;     // 0x288
    float freelook_duration_;        // 0x28C
    Vec3f freelook_snap_target_;     // 0x290
    uint32_t auto_rotate_start_ms_;  // 0x29C
    unk_t _unk_2A0;
    unk_t _unk_2A4;

    uint32_t fov_alt_blend_start_ms_;  // 0x2A8
    float fov_alt_blend_duration_;     // 0x2AC
    float fov_blend_alt_from_;         // 0x2B0
    float fov_blend_alt_to_;           // 0x2B4
    float fov_blend_alt_end_;          // 0x280
    float fov_blend_alt_start_;        // 0x284

    float fov_offset_scale_;      // 0x2C0
    uint32_t zoom_profile_id_;    // 0x2C4
    float profile_zoom_dist_[2];  // 0x2C8
    unk_t _unk_2D0[8];            // 0x2D0

    unk_t _unk_2F0;                 // 0x2F0
    uintptr_t m2scene_node_;        // 0x2F4
    uintptr_t m2scene_prev_;        // 0x2F8
    uint32_t blend_style_init_;     // 0x2FC
    uintptr_t list_next_;           // 0x300
    CameraShake* shake_list_head_;  // 0x304
    uintptr_t shake_list_tail_;     // 0x308

    float saved_view_index_;        // 0x30C
    float saved_zoom_dist_;         // 0x310
    float saved_pitch_limit_;       // 0x314
    float telescope_pitch_offset_;  // 0x318
    void* vehicle_cam_;             // 0x31C

    HOOKKIT_HOOK_HANDLE(get, 0x004F5960, hookkit::Conv::eCdecl, CGCamera*);
};
