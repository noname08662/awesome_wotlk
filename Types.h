#pragma once
#include "Enums.h"
#include <Windows.h>
#include <functional>
#include <algorithm>
#include <numbers>

// Types
struct Status;
struct CameraVtbl;

using guid_t = uint64_t;
using unk_t = uint32_t;
using NamePlateFlags = uint32_t;

struct lua_State;
using lua_Number = double;

using DummyCallback_t = void(*)();
using FunctionCallback_t = std::function<void()>;

constexpr double M_PI = std::numbers::pi;

constexpr float NDC_X = 0.80000001f;
constexpr float NDC_Y = 0.60000002f;

template<typename T>
struct TSGrowableArray {
    uint32_t m_capacity;
    uint32_t m_count;
    T* m_data;
    uint32_t m_granularity;
};

template<typename T>
struct TSList {
    T* m_next;
    T* m_rev;
    uint32_t m_status;
};

template<typename T>
struct TSHashTable {
    void* vmt;
    void* m_listHead;
    void* m_listTail;
    uint32_t m_numEntries;
    uint32_t m_rehashCounter;
    uint32_t m_bucketCount;
    uint32_t m_mask;
    TSList<T>* m_bucket;
    uint32_t m_unk20;
    uint32_t m_maxEntries;
};

template <typename T> struct Vec2D { T x, y; };
template <typename T> struct Vec3D { T x, y, z; };
template <typename T> struct Vec4D { T x, y, z, o; };
struct VecXYZ : Vec3D<float> {
    VecXYZ operator-(const VecXYZ& r) const { return { x - r.x, y - r.y, z - r.z }; }
    float distance(const VecXYZ& other) const {
        VecXYZ diff = (*this) - other;
        return std::sqrtf(std::powf(diff.x, 2) + std::powf(diff.y, 2) + std::powf(diff.z, 2));
    }
};

struct C3Vector {
    float X, Y, Z;
};

struct C4Quaternion {
    float X, Y, Z, W;
};

struct C33Matrix {
    float m[3][3];
};

struct C44Matrix {
    float m[4][4];
};

struct Flag96 {
    uint32_t part1;
    uint32_t part2;
    uint32_t part3;
};

struct TerrainClickEvent {
    guid_t m_guid;
    C3Vector m_pos;
    uint32_t m_button;
};

inline const char* idToStr[35] = {
    "INVTYPE_NON_EQUIP",              //  0
    "INVTYPE_HEAD",                   //  1
    "INVTYPE_NECK",                   //  2
    "INVTYPE_SHOULDER",               //  3
    "INVTYPE_BODY",                   //  4
    "INVTYPE_CHEST",                  //  5
    "INVTYPE_WAIST",                  //  6
    "INVTYPE_LEGS",                   //  7
    "INVTYPE_FEET",                   //  8
    "INVTYPE_WRIST",                  //  9
    "INVTYPE_HAND",                   // 10
    "INVTYPE_FINGER",                 // 11
    "INVTYPE_TRINKET",                // 12
    "INVTYPE_WEAPON",                 // 13
    "INVTYPE_SHIELD",                 // 14
    "INVTYPE_RANGED",                 // 15
    "INVTYPE_CLOAK",                  // 16
    "INVTYPE_2HWEAPON",               // 17
    "INVTYPE_BAG",                    // 18
    "INVTYPE_TABARD",                 // 19
    "INVTYPE_ROBE",                   // 20
    "INVTYPE_WEAPONMAINHAND",         // 21
    "INVTYPE_WEAPONOFFHAND",          // 22
    "INVTYPE_HOLDABLE",               // 23
    "INVTYPE_AMMO",                   // 24
    "INVTYPE_THROWN",                 // 25
    "INVTYPE_RANGEDRIGHT",            // 26
    "INVTYPE_QUIVER",                 // 27
    "INVTYPE_RELIC",                  // 28
    "INVTYPE_PROFESSION_TOOL",        // 29
    "INVTYPE_PROFESSION_GEAR",        // 30
    "INVTYPE_EQUIPABLESPELL_OFFENSIVE", // 31
    "INVTYPE_EQUIPABLESPELL_UTILITY",   // 32
    "INVTYPE_EQUIPABLESPELL_DEFENSIVE", // 33
    "INVTYPE_EQUIPABLESPELL_WEAPON"     // 34
};

struct DBCHeader {
    uint32_t m_junk[3];
    uint32_t m_maxIndex;
    uint32_t m_minIndex;
};

struct CDataChunk {
    CDataChunk* m_nextChunk;
    char m_data[0];
};

struct CDataAllocator {
    uint32_t m_blockSize;
    uint32_t m_blocksPerChunk;
    uint32_t m_activeCount;
    CDataChunk* m_chunkList;
    void* m_freeList;
};

struct CM2Model {
    unk_t unk_00[92];
    float m_red;            // 0x170
    float m_green;          // 0x174
    float m_blue;           // 0x178
    float m_alpha;          // 0x17C
};

struct ObjectEntry {
    guid_t m_guid;
    int m_type;
    int m_entry;
    float m_scaleX;
    int m_padding;
};
static_assert(sizeof(ObjectEntry) == 0x18);

struct UnitEntry : ObjectEntry {
    guid_t m_charm;
    guid_t m_summon;
    guid_t m_critter;
    guid_t m_charmedBy;
    guid_t m_summonedBy;
    guid_t m_createdBy;
    guid_t m_target;
    guid_t m_channelObject;
    uint32_t m_channelSpell;
    uint32_t m_bytes0;
    uint32_t m_health;
    uint32_t m_power[7];
    uint32_t m_maxHealth;
    uint32_t m_maxPower[7];
    uint32_t m_powerRegenFlatModifier[7];
    uint32_t m_powerRegenInterruptedFlatModifier[7];
    uint32_t m_level;
    uint32_t m_factionTemplate;
    uint32_t m_virtualItemSlotId[3];
    uint32_t m_flags;
    uint32_t m_flags2;
    float m_auraState;
    uint32_t m_baseAttackTime[2];
    uint32_t m_rangedAttackTime;
    float m_boundingRadius;
    float m_combatReach;
    uint32_t m_displayId;
    uint32_t m_nativeDisplayId;
    uint32_t m_mountDisplayId;
    float m_minDamage;
    float m_maxDamage;
    float m_minOffhandDamage;
    float m_maxOffhandDamage;
    uint32_t m_bytes1;
    uint32_t m_petNumber;
    uint32_t m_petNameTimestamp;
    uint32_t m_petExperience;
    uint32_t m_petNextLevelExp;
    uint32_t m_dynamicFlags;
    float m_modCastSpeed;
    uint32_t m_createdBySpell;
    uint32_t m_npc_flags;
    char unk0[254];
};
static_assert(sizeof(UnitEntry) == 0x250);

struct GameObjectEntry : ObjectEntry {
    uint64_t m_createdBy;
    uint32_t m_displayId;
    uint32_t m_flags;
    float m_parentRotation[4];
    struct { uint16_t low; uint16_t high; } m_dynamic;
    uint32_t m_faction;
    uint32_t m_level;
    uint32_t m_bytes1;
};
static_assert(sizeof(GameObjectEntry) == 0x48);

struct PlayerQuest {
    int a1, a2, a3, a4, a5;
};
static_assert(sizeof(PlayerQuest) == 0x14);

struct PlayerVisibleItem {
    int m_entryId;
    int m_enchant;
};
static_assert(sizeof(PlayerVisibleItem) == 0x8);

struct PlayerEntry : UnitEntry {
    guid_t m_duelArbiter;
    uint32_t m_flags_player;
    uint32_t m_guildId, m_guildRank;
    Flag96 m_bytes;
    uint32_t m_duelTeam;
    uint32_t m_guildTimestamp;
    PlayerQuest m_quests[25];
    PlayerVisibleItem m_visibleItems[19];
};

struct ItemCacheRec {
    int m_id;
    int m_class;
    int m_subClass;
    int m_unkInt;
    int m_displayId;
    int m_quality;
    int m_flagsAndFaction[2];
    int m_buyPrice;
    int m_sellPrice;
    int m_invType;
    int m_allowClass;
    int m_allowRace;
    int m_itemLvl;
    int m_reqLvl;
    int m_reqSkill;
    int m_reqSkillRank;
    int m_reqSpell;
    int m_reqHonor;
    int m_reqCityRank;
    int m_reqRepFaction;
    int m_reqRepRank;
    int m_maxCount;
    int m_stackable;
    int m_containerSlots;
    int m_statsCount;
    int m_stats[10][2];
    int m_scalingStatDistribution;
    int m_scalingStatValue;
    float m_sSDDmg1[2];
    float m_sSDDmg2[2];
    int m_sSDDmgType[2];
    int m_resistance[7];
    int m_delay;
    int m_ammoType;
    float m_rangedModRange;
    int m_spellId[5];
    int m_spellTrigger[5];
    int m_spellCharges[5];
    int m_spellCooldown[5];
    int m_spellCategory[5];
    int m_spellCatCooldown[5];
    int m_bonding;
    char* m_description;
    int m_pageTextId;
    int m_languageId;
    int m_pageMaterial;
    int m_startQuest;
    int m_lockId;
    int m_material;
    int m_sheath;
    int m_randomProperty;
    int m_randomSuffix;
    int m_block;
    int m_itemSetId;
    int m_maxDurability;
    int m_area;
    int m_map;
    int m_bagFamily;
    int m_totemCategory;
    int m_socketColor[3];
    int m_socketItem[3];
    int m_socketBonus;
    int m_gemProperties;
    int m_reqDisenchantSkill;
    float m_armorDmgMod;
    int m_duration;
    int m_itemLimitCat;
    int m_holiday;
    char m_name[4][400];
};
static_assert(sizeof(ItemCacheRec) == 0x834);

struct ItemClassRec {
    uint32_t m_classID;
    uint32_t m_subclassMapID;
    uint32_t m_flags;
    char* m_className_lang;
};

struct ItemSubClassRec {
    uint32_t m_classID;
    uint32_t m_subClassID;
    uint32_t m_prerequisiteProficiency;
    uint32_t m_postrequisiteProficiency;
    uint32_t m_flags;
    uint32_t m_displayFlags;
    uint32_t m_weaponParrySeq;
    uint32_t m_weaponReadySeq;
    uint32_t m_weaponAttackSeq;
    uint32_t m_WeaponSwingSize;
    char* m_displayName_lang;
    char* m_verboseName_lang;
};

struct ItemDisplayInfoRec {
    uint32_t m_ID;
    char* m_modelName[2];
    char* m_modelTexture[2];
    char* m_inventoryIcon;
    uint32_t m_groundModel;
    uint32_t m_geosetGroup[3];
    uint32_t m_spellVisualID;
    uint32_t m_groupSoundIndex;
    uint32_t m_helmetGeosetVisID[2];
    uint32_t m_texture[8];
    uint32_t m_itemVisual;
};

struct SpellRec {
    uint32_t m_ID;
    uint32_t m_category;
    uint32_t m_dispel;
    int32_t m_mechanic;

    uint32_t m_attributes;
    uint32_t m_attributesEx;
    uint32_t m_attributesEx2;
    uint32_t m_attributesEx3;
    uint32_t m_attributesEx4;
    uint32_t m_attributesEx5;
    uint32_t m_attributesEx6;
    uint32_t m_attributesEx7;

    uint32_t m_stances;
    unk_t unk_34;
    uint32_t m_stancesNot;
    unk_t unk_3C;
    uint32_t m_targets;
    uint32_t m_targetCreatureType;
    uint32_t m_requiresSpellFocus;
    uint32_t m_facingCasterFlags;
    uint32_t m_casterAuraState;
    uint32_t m_targetAuraState;
    uint32_t m_casterAuraStateNot;
    uint32_t m_targetAuraStateNot;
    uint32_t m_casterAuraSpell;
    uint32_t m_targetAuraSpell;
    uint32_t m_excludeCasterAuraSpell;
    uint32_t m_excludeTargetAuraSpell;
    uint32_t m_castingTimeIndex;
    uint32_t m_recoveryTime;
    uint32_t m_categoryRecoveryTime;
    uint32_t m_interruptFlags;
    uint32_t m_auraInterruptFlags;
    uint32_t m_channelInterruptFlags;
    uint32_t m_procFlags;
    uint32_t m_procChance;
    uint32_t m_procCharges;
    uint32_t m_maxLevel;
    uint32_t m_baseLevel;
    uint32_t m_spellLevel;
    uint32_t m_durationIndex;
    int32_t m_powerType;
    uint32_t m_manaCost;
    uint32_t m_manaCostPerlevel;
    uint32_t m_manaPerSecond;
    uint32_t m_manaPerSecondPerLevel;
    uint32_t m_rangeIndex;
    float m_speed;
    uint32_t m_modalNextSpell;
    uint32_t m_stackAmount;
    uint32_t m_totem[2];
    int32_t m_reagent[8];
    uint32_t m_reagentCount[8];
    int32_t m_equippedItemClass;
    int32_t m_equippedItemSubClassMask;
    int32_t m_equippedItemInventoryTypeMask;

    int32_t m_effect[3];
    int32_t m_effectDieSides[3];
    //int32_t m_effectBaseDice[3];
    //float m_effectDicePerLevel[3];
    float m_effectRealPointsPerLevel[3];
    int32_t m_effectBasePoints[3];
    uint32_t m_effectMechanic[3];
    uint32_t m_effectImplicitTargetA[3];
    uint32_t m_effectImplicitTargetB[3];
    uint32_t m_effectRadiusIndex[3];
    uint32_t m_effectApplyAuraName[3];
    uint32_t m_effectAmplitude[3];
    float m_effectMultipleValue[3];
    uint32_t m_effectChainTarget[3];
    uint32_t m_effectItemType[3];
    int32_t m_effectMiscValue[3];
    int32_t m_effectMiscValueB[3];
    uint32_t m_effectTriggerSpell[3];
    float m_effectPointsPerComboPoint[3];
    Flag96 m_effectSpellClassMask[3];

    uint32_t m_spellVisual[2];
    uint32_t m_spellIconID;
    uint32_t m_activeIconID;
    uint32_t m_spellPriority;

    uint32_t m_spellNameOffset;  // m_string m_block
    uint32_t m_rankOffset;
    uint32_t m_descriptionOffset;
    uint32_t m_toolTipOffset;

    uint32_t m_manaCostPercentage;
    uint32_t m_startRecoveryCategory;
    uint32_t m_startRecoveryTime;
    uint32_t m_maxTargetLevel;
    uint32_t m_spellFamilyName;
    Flag96 m_spellFamilyFlags;
    uint32_t m_maxAffectedTargets;
    uint32_t m_dmgClass;
    uint32_t m_preventionType;
    uint32_t m_stanceBarOrder;

    float m_dmgMultiplier[3];
    uint32_t m_minFactionId;
    uint32_t m_minReputation;
    uint32_t m_requiredAuraVision;
    uint32_t m_totemCategory[2];
    int32_t m_areaGroupId;
    int32_t m_schoolMask;
    uint32_t m_runeCostID;
    uint32_t m_spellMissileID;
    uint32_t m_powerDisplayId;
    unk_t unk_294[3];
    uint32_t m_spellDescriptionVariableID;
    uint32_t m_spellDifficultyId;
};
static_assert(sizeof(SpellRec) == 0x2A8);

struct CreatureCache {
    uint32_t m_ID;
    char* m_subnameptr;
    char* m_iconnameptr;
    uint32_t m_typeflags;
    ETypeMask m_type;
    uint32_t m_family;
    uint32_t m_rank;
    int m_killcredit[2];
    int m_displayid[4];
    float m_hpmodifier;
    float m_mpmodifier;
    char m_racialleader[4];
    int m_questitem[6];
    int m_movementid;
    char m_name[4][1024];
    char m_subname[1024];
    char m_iconname[1024];
};
static_assert(sizeof(CreatureCache) == 0x185C);

struct FactionTemplateRec {
    uint32_t m_ID;
    uint32_t m_faction;
    uint32_t m_flags;
    uint32_t m_factionGroup;
    uint32_t m_friendGroup;
    uint32_t m_enemyGroup;
    uint32_t m_enemies[4];
    uint32_t m_friend[4];
};
static_assert(sizeof(FactionTemplateRec) == 0x38);

struct BattlegroundData {
    uint32_t m_instanceID;      // 0x00
    uint32_t m_mapID;           // 0x04
    unk_t unk_08;               // 0x08
    unk_t unk_0C;               // 0x0C
    unk_t unk_10;               // 0x10
    unk_t unk_14;               // 0x14
    unk_t unk_18;               // 0x18
    unk_t unk_1C;               // 0x1C
    uint32_t m_teamID;          // 0x20
};

struct __declspec(novtable) XMLObject {
    unk_t unk_00[0x38 / 4];

    using Constructor_t = XMLObject * (__thiscall*)(XMLObject*, int, const char*);
    using SetValue_t = void(__thiscall*)(XMLObject*, const char*, const char*);

    XMLObject(int a1, const char* parentName) { (reinterpret_cast<Constructor_t>(0x00814AD0))(this, a1, parentName); }
	void setValue(const char* key, const char* value) { (reinterpret_cast<SetValue_t>(0x00814C40))(this, key, value); }
};