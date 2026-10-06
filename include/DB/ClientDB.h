#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <string_view>

namespace client_db {
template <typename Rec>
class DB {
public:
    virtual ~DB() = 0;
    virtual void load(const char* filename, int line_number) = 0;
    virtual void nullsub() = 0;
    virtual void loadRecords(void* file_handle, const char* filename, int line_number) = 0;
    virtual bool copyRowAtIndex(int index, Rec* out_row) = 0;
    virtual void free() = 0;
    virtual void reset() = 0;
    virtual const Rec** getRows() = 0;

    int32_t is_loaded_;
    int32_t num_rows_;
    int32_t max_index_;
    int32_t min_index_;
    int32_t string_table_;
};

template <typename Rec>
class IDBRec {
public:
    virtual ~IDBRec() = 0;
    virtual const Rec* getRecord(int id) = 0;
};

template <typename Rec>
class DBRec : public DB<Rec>, public IDBRec<Rec> {
public:
    const Rec* first_row_;
    const Rec** rows_;
};

struct DbcEntry {
    std::string_view name;
    uintptr_t address;
};

inline constexpr DbcEntry kDbSrc[] = {
    {"achievement", 0x00AD305C},
    {"achievement_Criteria", 0x00AD3080},
    {"achievement_Category", 0x00AD30A4},
    {"animationData", 0x00AD30C8},
    {"areaGroup", 0x00AD30EC},
    {"areaPOI", 0x00AD3110},
    {"areaTable", 0x00AD3134},
    {"areaTrigger", 0x00AD3158},
    {"attackAnimKits", 0x00AD317C},
    {"attackAnimTypes", 0x00AD31A0},
    {"auctionHouse", 0x00AD31C4},
    {"bankBagSlotPrices", 0x00AD31E8},
    {"bannedAddOns", 0x00AD320C},
    {"barberShopStyle", 0x00AD3230},
    {"battlemasterList", 0x00AD3254},
    {"cameraShakes", 0x00AD3278},
    {"cfg_Categories", 0x00AD329C},
    {"cfg_Configs", 0x00AD32C0},
    {"charBaseInfo", 0x00AD32E4},
    {"charHairGeosets", 0x00AD3308},
    {"charSections", 0x00AD332C},
    {"charStartOutfit", 0x00AD3350},
    {"charTitles", 0x00AD3374},
    {"characterFacialHairStyles", 0x00AD3398},
    {"chatChannels", 0x00AD33BC},
    {"chatProfanity", 0x00AD33E0},
    {"chrClasses", 0x00AD3404},
    {"chrRaces", 0x00AD3428},
    {"cinematicCamera", 0x00AD344C},
    {"cinematicSequences", 0x00AD3470},
    {"creatureDisplayInfoExtra", 0x00AD3494},
    {"creatureDisplayInfo", 0x00AD34B8},
    {"creatureFamily", 0x00AD34DC},
    {"creatureModelData", 0x00AD3500},
    {"creatureMovementInfo", 0x00AD3524},
    {"creatureSoundData", 0x00AD3548},
    {"creatureSpellData", 0x00AD356C},
    {"creatureType", 0x00AD3590},
    {"currencyTypes", 0x00AD35B4},
    {"currencyCategory", 0x00AD35D8},
    {"danceMoves", 0x00AD35FC},
    {"deathThudLookups", 0x00AD3620},
    {"declinedWord", 0x00AD3644},
    {"declinedWordCases", 0x00AD3668},
    {"destructibleModelData", 0x00AD368C},
    {"dungeonEncounter", 0x00AD36B0},
    {"dungeonMap", 0x00AD36D4},
    {"dungeonMapChunk", 0x00AD36F8},
    {"durabilityCosts", 0x00AD371C},
    {"durabilityQuality", 0x00AD3740},
    {"emotes", 0x00AD3764},
    {"emotesTextData", 0x00AD3788},
    {"emotesTextSound", 0x00AD37AC},
    {"emotesText", 0x00AD37D0},
    {"environmentalDamage", 0x00AD37F4},
    {"exhaustion", 0x00AD3818},
    {"factionGroup", 0x00AD383C},
    {"faction", 0x00AD3860},
    {"factionTemplate", 0x00AD3884},
    {"fileData", 0x00AD38A8},
    {"footprintTextures", 0x00AD38CC},
    {"footstepTerrainLookup", 0x00AD38F0},
    {"gameObjectArtKit", 0x00AD3914},
    {"gameObjectDisplayInfo", 0x00AD3938},
    {"gameTables", 0x00AD395C},
    {"gameTips", 0x00AD3980},
    {"gemProperties", 0x00AD39A4},
    {"glyphProperties", 0x00AD39C8},
    {"glyphSlot", 0x00AD39EC},
    {"gMSurveyAnswers", 0x00AD3A10},
    {"gMSurveyCurrentSurvey", 0x00AD3A34},
    {"gMSurveyQuestions", 0x00AD3A58},
    {"gMSurveySurveys", 0x00AD3A7C},
    {"gMTicketCategory", 0x00AD3AA0},
    {"groundEffectDoodad", 0x00AD3AC4},
    {"groundEffectTexture", 0x00AD3AE8},
    {"gtBarberShopCostBase", 0x00AD3B0C},
    {"gtCombatRatings", 0x00AD3B30},
    {"gtChanceToMeleeCrit", 0x00AD3B54},
    {"gtChanceToMeleeCritBase", 0x00AD3B78},
    {"gtChanceToSpellCrit", 0x00AD3B9C},
    {"gtChanceToSpellCritBase", 0x00AD3BC0},
    {"gtNPCManaCostScaler", 0x00AD3BE4},
    {"gtOCTClassCombatRatingScalar", 0x00AD3C08},
    {"gtOCTRegenHP", 0x00AD3C2C},
    {"gtOCTRegenMP", 0x00AD3C50},
    {"gtRegenHPPerSpt", 0x00AD3C74},
    {"gtRegenMPPerSpt", 0x00AD3C98},
    {"helmetGeosetVisData", 0x00AD3CBC},
    {"holidayDescriptions", 0x00AD3CE0},
    {"holidayNames", 0x00AD3D04},
    {"holidays", 0x00AD3D28},
    {"item", 0x00AD3D4C},
    {"itemBagFamily", 0x00AD3D70},
    {"itemClass", 0x00AD3D94},
    {"itemCondExtCosts", 0x00AD3DB8},
    {"itemDisplayInfo", 0x00AD3DDC},
    {"itemExtendedCost", 0x00AD3E00},
    {"itemGroupSounds", 0x00AD3E24},
    {"itemLimitCategory", 0x00AD3E48},
    {"itemPetFood", 0x00AD3E6C},
    {"itemPurchaseGroup", 0x00AD3E90},
    {"itemRandomProperties", 0x00AD3EB4},
    {"itemRandomSuffix", 0x00AD3ED8},
    {"itemSet", 0x00AD3EFC},
    {"itemSubClassMask", 0x00AD3F20},
    {"itemSubClass", 0x00AD3F44},
    {"itemVisualEffects", 0x00AD3F68},
    {"itemVisuals", 0x00AD3F8C},
    {"languageWords", 0x00AD3FB0},
    {"languages", 0x00AD3FD4},
    {"lfgDungeonExpansion", 0x00AD3FF8},
    {"lfgDungeonGroup", 0x00AD401C},
    {"lfgDungeons", 0x00AD4040},
    {"liquidType", 0x00AD4064},
    {"liquidMaterial", 0x00AD4088},
    {"loadingScreens", 0x00AD40AC},
    {"loadingScreenTaxiSplines", 0x00AD40D0},
    {"lock", 0x00AD40F4},
    {"lockType", 0x00AD4118},
    {"mailTemplate", 0x00AD413C},
    {"map", 0x00AD4160},
    {"mapDifficulty", 0x00AD4184},
    {"material", 0x00AD41A8},
    {"movie", 0x00AD41CC},
    {"movieFileData", 0x00AD41F0},
    {"movieVariation", 0x00AD4214},
    {"nameGen", 0x00AD4238},
    {"nPCSounds", 0x00AD425C},
    {"namesProfanity", 0x00AD4280},
    {"namesReserved", 0x00AD42A4},
    {"overrideSpellData", 0x00AD42C8},
    {"package", 0x00AD42EC},
    {"pageTextMaterial", 0x00AD4310},
    {"paperDollItemFrame", 0x00AD4334},
    {"particleColor", 0x00AD4358},
    {"petPersonality", 0x00AD437C},
    {"powerDisplay", 0x00AD43A0},
    {"pvpDifficulty", 0x00AD43C4},
    {"questFactionReward", 0x00AD43E8},
    {"questInfo", 0x00AD440C},
    {"questSort", 0x00AD4430},
    {"questXP", 0x00AD4454},
    {"resistances", 0x00AD4478},
    {"randPropPoints", 0x00AD449C},
    {"scalingStatDistribution", 0x00AD44C0},
    {"scalingStatValues", 0x00AD44E4},
    {"screenEffect", 0x00AD4508},
    {"serverMessages", 0x00AD452C},
    {"sheatheSoundLookups", 0x00AD4550},
    {"skillCostsData", 0x00AD4574},
    {"skillLineAbility", 0x00AD4598},
    {"skillLineCategory", 0x00AD45BC},
    {"skillLine", 0x00AD45E0},
    {"skillRaceClassInfo", 0x00AD4604},
    {"skillTiers", 0x00AD4628},
    {"soundAmbience", 0x00AD464C},
    {"soundEntries", 0x00AD4670},
    {"soundEmitters", 0x00AD4694},
    {"soundProviderPreferences", 0x00AD46B8},
    {"soundSamplePreferences", 0x00AD46DC},
    {"soundWaterType", 0x00AD4700},
    {"spamMessages", 0x00AD4724},
    {"spellCastTimes", 0x00AD4748},
    {"spellCategory", 0x00AD476C},
    {"spellChainEffects", 0x00AD4790},
    {"spellDescriptionVariables", 0x00AD47B4},
    {"spellDifficulty", 0x00AD47D8},
    {"spellDispelType", 0x00AD47FC},
    {"spellDuration", 0x00AD4820},
    {"spellEffectCameraShakes", 0x00AD4844},
    {"spellFocusObject", 0x00AD4868},
    {"spellIcon", 0x00AD488C},
    {"spellItemEnchantment", 0x00AD48B0},
    {"spellItemEnchantmentCondition", 0x00AD48D4},
    {"spellMechanic", 0x00AD48F8},
    {"spellMissile", 0x00AD491C},
    {"spellMissileMotion", 0x00AD4940},
    {"spellRadius", 0x00AD4964},
    {"spellRange", 0x00AD4988},
    {"spellRuneCost", 0x00AD49AC},
    {"spell", 0x00AD49D0},
    {"spellShapeshiftForm", 0x00AD49F4},
    {"spellVisualEffectName", 0x00AD4A18},
    {"spellVisualKit", 0x00AD4A3C},
    {"spellVisualKitAreaModel", 0x00AD4A60},
    {"spellVisualKitModelAttach", 0x00AD4A84},
    {"spellVisual", 0x00AD4AA8},
    {"stableSlotPrices", 0x00AD4ACC},
    {"stationery", 0x00AD4AF0},
    {"stringLookups", 0x00AD4B14},
    {"summonProperties", 0x00AD4B38},
    {"talent", 0x00AD4B5C},
    {"talentTab", 0x00AD4B80},
    {"taxiNodes", 0x00AD4BA4},
    {"taxiPathNode", 0x00AD4BC8},
    {"taxiPath", 0x00AD4BEC},
    {"teamContributionPoints", 0x00AD4C10},
    {"terrainType", 0x00AD4C34},
    {"terrainTypeSounds", 0x00AD4C58},
    {"totemCategory", 0x00AD4C7C},
    {"transportAnimation", 0x00AD4CA0},
    {"transportPhysics", 0x00AD4CC4},
    {"transportRotation", 0x00AD4CE8},
    {"uISoundLookups", 0x00AD4D0C},
    {"unitBloodLevels", 0x00AD4D30},
    {"unitBlood", 0x00AD4D54},
    {"vehicle", 0x00AD4D78},
    {"vehicleSeat", 0x00AD4D9C},
    {"vehicleUIIndicator", 0x00AD4DC0},
    {"vehicleUIIndSeat", 0x00AD4DE4},
    {"vocalUISounds", 0x00AD4E08},
    {"WMOAreaTable", 0x00AD4E2C},
    {"weaponImpactSounds", 0x00AD4E50},
    {"weaponSwingSounds2", 0x00AD4E74},
    {"weather", 0x00AD4E98},
    {"worldMapArea", 0x00AD4EBC},
    {"worldMapContinent", 0x00AD4EE0},
    {"worldMapOverlay", 0x00AD4F04},
    {"worldMapTransforms", 0x00AD4F28},
    {"worldSafeLocs", 0x00AD4F4C},
    {"worldStateUI", 0x00AD4F70},
    {"zoneIntroMusicTable", 0x00AD4F94},
    {"zoneMusic", 0x00AD4FB8},
    {"worldStateZoneSounds", 0x00AD4FDC},
    {"worldChunkSounds", 0x00AD5000},
    {"soundEntriesAdvanced", 0x00AD5024},
    {"objectEffect", 0x00AD5048},
    {"objectEffectGroup", 0x00AD506C},
    {"objectEffectModifier", 0x00AD5090},
    {"objectEffectPackage", 0x00AD50B4},
    {"objectEffectPackageElem", 0x00AD50D8},
    {"soundFilter", 0x00AD50FC},
    {"soundFilterElem", 0x00AD5120},
};

struct DbcDatabase {
    constexpr DBRec<void>* operator[](std::string_view lookup_name) const {
        for (const auto& [name, address] : kDbSrc) {
            if (name == lookup_name) { return reinterpret_cast<DBRec<void>*>(address); }
        }
        return nullptr;
    }
};

inline constexpr DbcDatabase kGDb;

using DecompressRowFn = void(__cdecl*)(const void* src_row, uint32_t row_size, void* out_dst);
inline const auto kDecompressRow = reinterpret_cast<DecompressRowFn>(0x004CFBB0);
inline const auto kIsDecompressionActive = reinterpret_cast<const uint8_t*>(0x00C5DEA0);

template <typename Rec>
const Rec* getRowById(std::string_view db_name, uint32_t id) {
    auto* db = reinterpret_cast<const DBRec<Rec>*>(kGDb[db_name]);
    if (!db || !db->rows_) { return nullptr; }
    if (static_cast<int32_t>(id) < db->min_index_ || static_cast<int32_t>(id) > db->max_index_) { return nullptr; }
    return db->rows_[id - static_cast<uint32_t>(db->min_index_)];
}

template <typename Rec>
bool getRowById(std::string_view db_name, uint32_t id, Rec* out_row) {
    if (!out_row) { return false; }
    const Rec* raw_row = getRowById<Rec>(db_name, id);
    if (!raw_row) { return false; }
    *out_row = *raw_row;
    return true;
}

template <typename Rec, typename Predicate>
const Rec* findRow(std::string_view db_name, Predicate pred) {
    auto* db = reinterpret_cast<const DBRec<Rec>*>(kGDb[db_name]);
    if (!db || !db->rows_) { return nullptr; }
    uint32_t total_slots = db->max_index_ - db->min_index_ + 1;
    for (uint32_t i = 0; i < total_slots; ++i) {
        const Rec* row = db->rows_[i];
        if (!row) { continue; }
        if (pred(*row)) { return row; }
    }
    return nullptr;
}

template <typename Rec>
bool getLocalizedRowById(std::string_view db_name, uint32_t id, Rec* out_row) {
    if (!out_row) { return false; }
    auto* db = reinterpret_cast<const DBRec<Rec>*>(kGDb[db_name]);
    if (!db || !db->rows_) { return false; }
    if (static_cast<int32_t>(id) < db->min_index_ || static_cast<int32_t>(id) > db->max_index_) { return false; }
    const Rec* raw_row = db->rows_[id - static_cast<uint32_t>(db->min_index_)];
    if (!raw_row) { return false; }
    if (*kIsDecompressionActive != 0) {
        kDecompressRow(raw_row, sizeof(Rec), out_row);
    } else {
        std::memcpy(out_row, raw_row, sizeof(Rec));
    }
    return true;
}

template <typename Rec>
std::optional<Rec> getLocalizedRowById(std::string_view db_name, uint32_t id) {
    Rec row{};
    if (getLocalizedRowById<Rec>(db_name, id, &row)) { return row; }
    return std::nullopt;
}

template <typename Rec, typename Predicate>
std::optional<Rec> findLocalizedRow(std::string_view db_name, Predicate pred) {
    auto* db = reinterpret_cast<const DBRec<Rec>*>(kGDb[db_name]);
    if (!db || !db->rows_) { return std::nullopt; }
    uint32_t total_slots = db->max_index_ - db->min_index_ + 1;
    Rec temp_row{};
    for (uint32_t i = 0; i < total_slots; ++i) {
        const Rec* raw_row = db->rows_[i];
        if (!raw_row) { continue; }
        if (*kIsDecompressionActive != 0) {
            kDecompressRow(raw_row, sizeof(Rec), &temp_row);
        } else {
            std::memcpy(&temp_row, raw_row, sizeof(Rec));
        }
        if (pred(temp_row)) { return temp_row; }
    }
    return std::nullopt;
}
}  // namespace client_db
