#pragma once

#include <cstdint>

enum UIFrame {
    eCurrentFramePtr = 0x00B499A8,
    eCurrentFrameOffset = 0x78,
    eFirstFrame = 0x0CD4,
    eNextFrame = 0x0CCC,
    eUnkDivWidth = 0x00AC0CB4,
    eUnkDivHeight = 0x00AC0CB8,
    eScreenWidth = 0x00C7D2C8,
    eScreenHeight = 0x00C7D2C4,
    eFrameLeft = 0x68,
    eFrameRight = 0x70,
    eFrameTop = 0x6C,
    eFrameBottom = 0x64,
    eParentPtr = 0x94,
    eEffectiveScale = 0x7C,
    eName = 0x1C,
    eVisible = 0xE0,
};

enum TextStateFlags : uint32_t {
    eTextStateNone = 0x0,
    eTextStateJustifyHLeft = 0x40,
    eTextStateJustifyHCenter = 0x80,
    eTextStateJustifyHRight = 0x100,
    eTextStateJustifyVTop = 0x200,
    eTextStateJustifyVMiddle = 0x400,
    eTextStateJustifyVBottom = 0x800,
    eTextStateAutoWrap = 0x1000,
    eTextStateShadow = 0x2000,
    eTextStateOutline = 0x4000,
    eTextStateMonochrome = 0x8000,
    eTextStateThickOutline = 0x10000,
    eTextStateIndentFirst = 0x20000
};

enum FrameState : uint32_t {
    eFlagToplevel = 0x1,
    eFlagObscured = 0x10,
    eFlagMovable = 0x100,
    eFlagResizable = 0x200,
    eFlagDisabled = 0x400,
    eFlagUserPlaced = 0x1000,
    eFlagBeingScrolled = 0x2000,
    eFlagScrollRoot = 0x4000,
    eFlagIsButton = 0x10000,
    eFlagWorldRender = 0x40000,
    eFlagDontSave = 0x80000,
};
