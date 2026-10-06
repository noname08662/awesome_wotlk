#pragma once

#include <cstdint>

enum ClassId : uint32_t {
    eNone = 0x0,
    eWarrior = 0x1,
    ePaladin = 0x2,
    eHunter = 0x3,
    eRogue = 0x4,
    ePriest = 0x5,
    eDeathKnight = 0x6,
    eShaman = 0x7,
    eMage = 0x8,
    eWarlock = 0x9,
    eDruid = 0xB,
};

enum RaceId : uint32_t {
    eHuman = 0x1,
    eOrc = 0x2,
    eDwarf = 0x3,
    eNightElf = 0x4,
    eUndead = 0x5,
    eTauren = 0x6,
    eGnome = 0x7,
    eTroll = 0x8,
    eGoblin = 0x9,
    eBloodElf = 0xA,
    eDraenei = 0xB,
    eFelOrc = 0xC,
    eNaga = 0xD,
    eBroken = 0xE,
    eSkeleton = 0xF,
    eEnd = 0x10,
};

enum CharSectionType : uint32_t {
    eSectionTypeSkin = 0u,
    eSectionTypeFace = 1u,
    eSectionTypeFacialHair = 2u,
    eSectionTypeHair = 3u,
    eSectionTypeUnderwear = 4u,
};

enum CharSectionFlags : uint32_t { eSectionFlagPlayer = 0x1, eSectionFlagDeathKnight = 0x4 };

enum CharGender : uint32_t { eGenderMale = 0u, eGenderFemale = 1u, eGenderNone = 2u };

enum CharEquipmentSlot : uint32_t {
    eCharSlotHead = 0u,
    eCharSlotShoulders = 1u,
    eCharSlotShirt = 2u,
    eCharSlotChest = 3u,
    eCharSlotBelt = 4u,
    eCharSlotPants = 5u,
    eCharSlotBoots = 6u,
    eCharSlotBracers = 7u,
    eCharSlotGloves = 8u,
    eCharSlotTabard = 9u,
    eCharSlotCape = 10u,
    eCharSlotSpecial = 11u,
};

enum CharGeosetGroup : uint32_t {
    eCGGHairstyle = 0u,
    eCGGFaciaL1 = 1u,
    eCGGFaciaL2 = 2u,
    eCGGFaciaL3 = 3u,
    eCGGBracers = 4u,
    eCGGBootsLow = 5u,
    eCGGPants = 6u,
    eCGGEars = 7u,
    eCGGGloves = 8u,
    eCGGBootsHigh = 9u,
    eCGGChest = 10u,
    eCGGBelt = 11u,
    eCGGTabard = 12u,
    eCGGRobe = 13u,
    eCGGUnderwear = 14u,
    eCGGCape = 15u,
    eCGGFaciaL4 = 16u,
    eCGGFaciaL5 = 17u,
    eCGGMisc = 18u,
};

enum ComponentRegion : uint32_t {
    eCharRegionArmUpper = 0u,
    eCharRegionArmLower = 1u,
    eCharRegionHand = 2u,
    eCharRegionTorsoUpper = 3u,
    eCharRegionTorsoLower = 4u,
    eCharRegionLegUpper = 5u,
    eCharRegionLegLower = 6u,
    eCharRegionFoot = 7u,
    eCharRegionFaceUpper = 8u,
    eCharRegionFaceLower = 9u,
};
