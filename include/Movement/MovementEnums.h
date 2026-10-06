#pragma once

#include <cstdint>

enum MovementFlags : uint32_t {
    eMovementflagNone = 0x0,
    eMovementflagForward = 0x1,
    eMovementflagBackward = 0x2,
    eMovementflagStrafeLeft = 0x4,
    eMovementflagStrafeRight = 0x8,
    eMovementflagTurnLeft = 0x10,
    eMovementflagTurnRight = 0x20,
    eMovementflagPitchUp = 0x40,
    eMovementflagPitchDown = 0x80,
    eMovementflagWalking = 0x100,
    eMovementflagDisableGravity = 0x400,
    eMovementflagFalling = 0x1000,
    eMovementflagFallingFar = 0x2000,
    eMovementflagSwimming = 0x200000,
    eMovementflagAscending = 0x400000,
    eMovementflagDescending = 0x800000,
    eMovementflagFlying = 0x2000000,
    eMovementflagSplineEnabled = 0x8000000,
    eMovementflagSplineElevation = 0x20000000,
    eMovementflagDirMask = 0xF,
    eMovementflagMovingMask = 0xC0100F,
    eMovementflagActiveTickMask = 0xC010FF,
};

enum MovementFlagsExtra : uint16_t {
    eMovementflag2FeatherFall = 0x4,
    eMovementflag2FullSpeedPitch = 0x20,
};

enum MoveEventType {
    eMoveEventForceRunSpeedChange = 0x17,
    eMoveEventForceRunBackSpeedChange = 0x18,
    eMoveEventForceWalkSpeedChange = 0x19,
    eMoveEventForceSwimSpeedChange = 0x1A,
    eMoveEventForceSwimBackSpeedChange = 0x1B,
    eMoveEventForceFlightSpeedChange = 0x1C,
    eMoveEventForceFlightBackSpeedChange = 0x1D,
    eMoveEventForceTurnRateChange = 0x1E,
    eMoveEventForcePitchRateChange = 0x1F,
    eMoveEventGravityEnable = 0x20,
    eMoveEventGravityDisable = 0x21,
    eMoveEventKnockBack = 0x22,
    eMoveEventFeatherFallEnable = 0x23,
    eMoveEventFeatherFallDisable = 0x24,
    eMoveEventHoverEnable = 0x25,
    eMoveEventHoverDisable = 0x26,
    eMoveEventWaterWalkEnable = 0x27,
    eMoveEventWaterWalkDisable = 0x28,
};

enum SplineFlags : uint32_t {
    eSplineflagFlying = 0x200,
    eSplineflagDone = 0x400,
    eSplineflagFalling = 0x800,
    eSplineflagCatmullrom = 0x2000,
};
