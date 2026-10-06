#pragma once

#include <cstdint>

#include "BaseTypes.h"

#include "Movement/C3Spline.h"
#include "Movement/MovementEnums.h"

class CGUnit_C;

class CMovement {
public:
    TSLink<CMovement> mov_link_;
    guid_t mov_guid_;
    Vec3f mov_pos_;
    unk_t _unused;
    float mov_facing_;
    float mov_pitch_;
    guid_t* mov_guid_ptr_;
    uint8_t mov_flag_;
    char _pad[3];
};

static_assert(sizeof(CMovement) == 0x30);

class CMovementShared : public CMovement {
public:
    TSLink<CMovementShared> link_;
    Vec3f spline_up_vector_;
    MovementFlags move_flags_;
    MovementFlagsExtra move_flags_extra_;
    uint8_t transport_seat_;
    char _pad;
    Vec3f vec_;
    float facing_;
    float pitch_;
    uint32_t n250;
    Vec3f spline_tangent_;
    float cos_facing_;
    float sin_facing_;
    float cos_pitch_;
    float sin_pitch_;
    uint32_t last_move_time_;
    float fall_start_elevation_;
    uint32_t n3276850;
    float cur_speed_;
    float walk_speed_;
    float run_speed_;
    float run_back_speed_;
    float swim_speed_;
    float swim_back_speed_;
    float flight_speed_;
    float flight_back_speed_;
    float turn_speed_;
    float _float;
    float _field_B4;
    float jump_initial_z_speed_;
    CMoveSpline* spline_;
    uint32_t timestamp_;
    unk_t _alignment;
};

static_assert(sizeof(CMovementShared) == 0xC8);

class CUnitMovement : public CMovementShared {
public:
    struct PlayerMoveEvent {
        TSLink<PlayerMoveEvent> link;
        uint32_t time;
        MoveEventType event_type;
        Vec3f pos;
        float facing;
        float pitch;
        unk_t unk;
        guid_t transport_guid;
        MovementFlags transport_flags;
        MovementFlagsExtra transport_flags_extra;
        char _pad[2];
        int move_time;
        float speed;
        int unk2;
        int horz_speed;
        int vert_speed;
        int counter;
        int8_t has_movement_info;
        int8_t flag;
        int8_t transport_seat;
        int8_t unk3;
        unk_t unk4;
    };

    static_assert(sizeof(PlayerMoveEvent) == 0x58);

    float bounding_radius_;
    float bounding_height_;
    float bounding_scale_;
    Vec3f cur_velocity_;
    unk_t _unkD8[2];
    int32_t history_buf_[16];
    uint32_t _unk0;
    uint32_t n50;
    uint32_t field_00000130;
    unk_t unk;
    TSExplicitList<PlayerMoveEvent> pmove_list_;
    CGUnit_C* unit_;
};

static_assert(sizeof(CUnitMovement) == 0x148);
