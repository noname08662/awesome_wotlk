#pragma once

enum M2ReplaceableTextureId {
    eSkin = 0x1,
    eObjectSkin = 0x2,
    eWeaponBlade = 0x3,
    eWeaponHandle = 0x4,
    eEnvironment = 0x5,
    eCharacterHair = 0x6,
    eCharacterFacialHair = 0x7,
    eSkinExtra = 0x8,
    eUiSkin = 0x9,
    eTaurenMane = 0xA,
    eMonster1 = 0xB,
    eMonster2 = 0xC,
    eMonster3 = 0xD,
    eItemIcon = 0xE,
};

enum M2BatchShaderFlags : uint16_t {
    eShaderBit03 = 0x8,
    eShaderBit14 = 0x4000,
    eShaderExplicit = 0x8000,
};

enum M2BatchFlags : uint8_t {
    eMultiTextureChain = 0x40,
};

enum M2Blend : uint32_t {
    eBlendOpaque = 0x0,
    eBlendAlphaKey = 0x1,
    eBlendAlpha = 0x2,
    eBlendNoAlphaAdd = 0x3,
    eBlendAdd = 0x4,
    eBlendMod = 0x5,
    eBlendMod2X = 0x6,
    eBlendCount = 0x7,
};

enum M2Combiner : uint32_t {
    eCombinerOpaque = 0x0,
    eCombinerMod = 0x1,
    eCombinerDecal = 0x2,
    eCombinerAdd = 0x3,
    eCombinerMod2X = 0x4,
    eCombinerFade = 0x5,
    eCombinerMod2XNa = 0x6,
    eCombinerAddNa = 0x7,
    eCombinerOpMask = 0x7,
    eCombinerEnvmap = 0x8,
    eCombinerStageShift = 0x4,
};

enum M2LightType : uint32_t {
    eLight0 = 0,
    eLight1 = 1,
};

enum M2Pass : uint32_t {
    ePass0 = 0,
    ePass1 = 1,
    ePass2 = 2,
    ePassCount = 3,
};

enum M2ModelRaycastMode {
    eRaycastModeNone = 0x0,
    eRaycastMode1 = 0x1,
    eRaycastMode2 = 0x2,
    eRaycastModeTransformed = 0x3,
};
