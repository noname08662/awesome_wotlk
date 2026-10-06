#pragma once

#include <cstdint>

enum UnitFlags : uint32_t {
    eUnitFlagServerControlled = 0x00000001,     // movement driven by server splines (with eUnitFlagStunned)
    eUnitFlagNonAttackable = 0x00000002,        // set while casting a spawn spell
    eUnitFlagRemoveClientControl = 0x00000004,  // legacy, superseded by SMSG_CLIENT_CONTROL
    eUnitFlagPlayerControlled = 0x00000008,
    eUnitFlagRename = 0x00000010,
    eUnitFlagPreparation = 0x00000020,  // arena/bg preparation, spells cost no reagents
    eUnitFlagUnk6 = 0x00000040,
    eUnitFlagNotAttackable1 = 0x00000080,  // with eUnitFlagPlayerControlled: non-pvp attackable
    eUnitFlagImmuneToPc = 0x00000100,      // no combat/assistance with players
    eUnitFlagImmuneToNpc = 0x00000200,     // no combat/assistance with npcs
    eUnitFlagLooting = 0x00000400,         // loot animation
    eUnitFlagPetInCombat = 0x00000800,     // pets: chasing a target; other units: a minion is in combat
    eUnitFlagPvpEnabling = 0x00001000,
    eUnitFlagSilenced = 0x00002000,
    eUnitFlagCantSwim = 0x00004000,
    eUnitFlagCanSwim = 0x00008000,         // shows the swim animation in water
    eUnitFlagNonAttackable2 = 0x00010000,  // removes the attackable icon
    eUnitFlagPacified = 0x00020000,
    eUnitFlagStunned = 0x00040000,
    eUnitFlagInCombat = 0x00080000,
    eUnitFlagOnTaxi = 0x00100000,    // blocks casting spells not allowed in flight
    eUnitFlagDisarmed = 0x00200000,  // blocks melee spells ("Requires melee weapon")
    eUnitFlagConfused = 0x00400000,
    eUnitFlagFleeing = 0x00800000,
    eUnitFlagPossessed = 0x01000000,  // under direct client control by a player (possess or vehicle)
    eUnitFlagUninteractible = 0x02000000,
    eUnitFlagSkinnable = 0x04000000,
    eUnitFlagMount = 0x08000000,
    eUnitFlagUnk28 = 0x10000000,
    eUnitFlagPreventEmotesFromChatText = 0x20000000,  // no automatic emotes from chat text ("lol", trailing ?/!, /yell)
    eUnitFlagSheathe = 0x40000000,
    eUnitFlagImmune = 0x80000000,  // immune to damage

    eUnitFlagDisallowed = (eUnitFlagServerControlled | eUnitFlagNonAttackable | eUnitFlagRemoveClientControl |
        eUnitFlagPlayerControlled | eUnitFlagRename | eUnitFlagPreparation | /* eUnitFlagUnk6 | */
        eUnitFlagNotAttackable1 | eUnitFlagLooting | eUnitFlagPetInCombat | eUnitFlagPvpEnabling | eUnitFlagSilenced |
        eUnitFlagNonAttackable2 | eUnitFlagPacified | eUnitFlagStunned | eUnitFlagInCombat | eUnitFlagOnTaxi |
        eUnitFlagDisarmed | eUnitFlagConfused | eUnitFlagFleeing | eUnitFlagPossessed | eUnitFlagSkinnable |
        eUnitFlagMount | eUnitFlagUnk28 | eUnitFlagPreventEmotesFromChatText | eUnitFlagSheathe | eUnitFlagImmune),

    eUnitFlagAllowed = (0xFFFFFFFF & ~eUnitFlagDisallowed)
};

enum NPCFlags : uint32_t {
    eUnitNPCFlagNone = 0x00000000,
    eUnitNPCFlagGossip = 0x00000001,
    eUnitNPCFlagQuestgiver = 0x00000002,
    eUnitNPCFlagUnk1 = 0x00000004,
    eUnitNPCFlagUnk2 = 0x00000008,
    eUnitNPCFlagTrainer = 0x00000010,
    eUnitNPCFlagTrainerClass = 0x00000020,
    eUnitNPCFlagTrainerProfession = 0x00000040,
    eUnitNPCFlagVendor = 0x00000080,
    eUnitNPCFlagVendorAmmo = 0x00000100,
    eUnitNPCFlagVendorFood = 0x00000200,
    eUnitNPCFlagVendorPoison = 0x00000400,
    eUnitNPCFlagVendorReagent = 0x00000800,
    eUnitNPCFlagRepair = 0x00001000,
    eUnitNPCFlagFlightmaster = 0x00002000,
    eUnitNPCFlagSpirithealer = 0x00004000,
    eUnitNPCFlagSpiritguide = 0x00008000,
    eUnitNPCFlagInnkeeper = 0x00010000,
    eUnitNPCFlagBanker = 0x00020000,
    eUnitNPCFlagPetitioner = 0x00040000,
    eUnitNPCFlagTabarddesigner = 0x00080000,
    eUnitNPCFlagBattlemaster = 0x00100000,
    eUnitNPCFlagAuctioneer = 0x00200000,
    eUnitNPCFlagStablemaster = 0x00400000,
    eUnitNPCFlagGuildBanker = 0x00800000,
    eUnitNPCFlagSpellclick = 0x01000000,
    eUnitNPCFlagPlayerVehicle = 0x02000000,
    eUnitNPCFlagMailbox = 0x04000000
};

enum TypeMask : uint32_t {
    eTypemaskObject = 0x1,
    eTypemaskItem = 0x2,
    eTypemaskContainer = 0x4,
    eTypemaskUnit = 0x8,
    eTypemaskPlayer = 0x10,
    eTypemaskGameobject = 0x20,
    eTypemaskDynamicobject = 0x40,
    eTypemaskCorpse = 0x80,
};

enum TypeId : int8_t {
    eTypeidObject = 0,
    eTypeidItem = 1,
    eTypeidContainer = 2,
    eTypeidUnit = 3,
    eTypeidPlayer = 4,
    eTypeidGameobject = 5,
    eTypeidDynamicobject = 6,
    eTypeidCorpse = 7,
    eNumTypeids = 8
};

enum GameobjectTypes : int8_t {
    eGameobjectTypeDoor = 0,
    eGameobjectTypeButton = 1,
    eGameobjectTypeQuestgiver = 2,
    eGameobjectTypeChest = 3,
    eGameobjectTypeBinder = 4,
    eGameobjectTypeGeneric = 5,
    eGameobjectTypeTrap = 6,
    eGameobjectTypeChair = 7,
    eGameobjectTypeSpellFocus = 8,
    eGameobjectTypeText = 9,
    eGameobjectTypeGoober = 10,
    eGameobjectTypeTransport = 11,
    eGameobjectTypeAreadamage = 12,
    eGameobjectTypeCamera = 13,
    eGameobjectTypeMapObject = 14,
    eGameobjectTypeMoTransport = 15,
    eGameobjectTypeDuelArbiter = 16,
    eGameobjectTypeFishingnode = 17,
    eGameobjectTypeSummoningRitual = 18,
    eGameobjectTypeMailbox = 19,
    eGameobjectTypeDoNotUse = 20,
    eGameobjectTypeGuardpost = 21,
    eGameobjectTypeSpellcaster = 22,
    eGameobjectTypeMeetingstone = 23,
    eGameobjectTypeFlagstand = 24,
    eGameobjectTypeFishinghole = 25,
    eGameobjectTypeFlagdrop = 26,
    eGameobjectTypeMiniGame = 27,
    eGameobjectTypeDoNotUse2 = 28,
    eGameobjectTypeCapturePoint = 29,
    eGameobjectTypeAuraGenerator = 30,
    eGameobjectTypeDungeonDifficulty = 31,
    eGameobjectTypeBarberChair = 32,
    eGameobjectTypeDestructibleBuilding = 33,
    eGameobjectTypeGuildBank = 34,
    eGameobjectTypeTrapdoor = 35
};

enum UnitReaction {
    eReactionHated = 1,
    eReactionHostile = 2,
    eReactionUnfriendly = 3,
    eReactionNeutral = 4,
    eReactionFriendly = 5,
    eReactionHonored = 6,
    eReactionRevered = 7,
    eReactionExalted = 8
};

enum UnitDynFlags {
    eUnitDynflagNone = 0x0000,
    eUnitDynflagLootable = 0x0001,
    eUnitDynflagTrackUnit = 0x0002,
    eUnitDynflagTapped = 0x0004,
    eUnitDynflagTappedByPlayer = 0x0008,
    eUnitDynflagSpecialinfo = 0x0010,
    eUnitDynflagDead = 0x0020,
    eUnitDynflagReferAFriend = 0x0040,
    eUnitDynflagTappedByAllThreatList = 0x0080
};

enum CreatureRank {
    eRankNormal = 0,
    eRankElite = 1,
    eRankRareelite = 2,
    eRankWorldboss = 3,
    eRankRare = 4,
    eRankTrivial = 5
};

enum ShapeshiftForm : uint32_t {
    eFormNone = 0x00,
    eFormCat = 0x01,
    eFormTree = 0x02,
    eFormTravel = 0x03,
    eFormAqua = 0x04,
    eFormBear = 0x05,
    eFormAmbient = 0x06,
    eFormGhoul = 0x07,
    eFormDirebear = 0x08,
    eFormStevesGhoul = 0x09,
    eFormTharonjaSkeleton = 0x0A,
    eFormTestOfStrength = 0x0B,
    eFormBlbPlayer = 0x0C,
    eFormShadowDance = 0x0D,
    eFormCreaturebear = 0x0E,
    eFormCreaturecat = 0x0F,
    eFormGhostwolf = 0x10,
    eFormBattlestance = 0x11,
    eFormDefensivestance = 0x12,
    eFormBerserkerstance = 0x13,
    eFormTest = 0x14,
    eFormZombie = 0x15,
    eFormMetamorphosis = 0x16,
    eFormUndead = 0x19,
    eFormMasterAngler = 0x1A,
    eFormFlightEpic = 0x1B,
    eFormShadow = 0x1C,
    eFormFlight = 0x1D,
    eFormStealth = 0x1E,
    eFormMoonkin = 0x1F,
    eFormSpiritofredemption = 0x20
};
