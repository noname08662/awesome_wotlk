#pragma once

#include <cstdint>

#include "BaseTypes.h"

struct ObjectEntry {
    guid_t guid;
    int type;
    int entry;
    float scale;
    uint32_t pad;
};

struct GameObjectEntry {
    uint64_t created_by;
    uint32_t display_id;
    uint32_t flags;
    float parent_rotation[4];

    struct {
        uint16_t low;
        uint16_t high;
    } dynamic;

    uint32_t faction;
    uint32_t level;
    uint8_t byte0;
    GameobjectTypes subtype;
    uint8_t byte3;
    uint8_t byte4;
};

struct DynamicObjectEntry {
    guid_t caster;
    uint8_t bytes0[4];
    uint32_t spellid;
    float radius;
    uint32_t casttime;
};

struct CorpseEntry {
    uint64_t owner;
    uint64_t party;
    uint32_t display_id;
    uint32_t item[19];
    uint8_t bytes1[4];
    uint8_t bytes2[4];
    uint32_t guild;
    uint32_t flags;
    uint32_t dynamic_flags;
    uint32_t pad;
};

struct PlayerEntry {
    struct PlayerQuest {
        int a1, a2, a3, a4, a5;
    };

    struct PlayerVisibleItem {
        int entry_id;
        int enchant;
    };

    guid_t duel_arbiter;
    uint32_t flags_player;
    uint32_t guild_id;
    uint32_t guild_rank;
    Flag96 bytes;
    uint32_t duel_team;
    uint32_t guild_timestamp;
    PlayerQuest quests[25];
    PlayerVisibleItem visible_items[19];
    uint32_t chosen_title;
    uint32_t fake_inebriation;
    uint32_t _alignment;
    uint64_t inv_slot_head[23];
    uint64_t pack_slot1[16];
    uint64_t bank_slot1[28];
    uint64_t bankbag_slot1[7];
    uint64_t vendorbuyback_slot1[12];
    uint64_t keyring_slot1[32];
    uint64_t currencytoken_slot1[32];
    uint64_t farsight;
    uint64_t known_titles;
    uint64_t known_titles1;
    uint64_t known_titles2;
    uint64_t known_currencies;
    uint32_t xp;
    uint32_t next_level_xp;
    uint16_t skill_info11[768];
    uint32_t character_points1;
    uint32_t character_points2;
    uint32_t track_creatures;
    uint32_t track_resources;
    float block_percentage;
    float dodge_percentage;
    float parry_percentage;
    uint32_t expertise;
    uint32_t offhand_expertise;
    float crit_percentage;
    float ranged_crit_percentage;
    float offhand_crit_percentage;
    float spell_crit_percentage1[7];
    uint32_t shield_block;
    float shield_block_crit_percentage;
    uint8_t explored_zones1[512];
    uint32_t rest_state_experience;
    uint32_t coinage;
    uint32_t mod_damage_done_pos[7];
    uint32_t mod_damage_done_neg[7];
    uint32_t mod_damage_done_pct[7];
    uint32_t mod_healing_done_pos;
    float mod_healing_pct;
    float mod_healing_done_pct;
    uint32_t mod_target_resistance;
    uint32_t mod_target_physical_resistance;
    uint8_t field_bytes[4];
    uint32_t ammo_id;
    uint32_t self_res_spell;
    uint32_t pvp_medals;
    uint32_t buyback_price1[12];
    uint32_t buyback_timestamp1[12];
    uint16_t kills[2];
    uint32_t today_contribution;
    uint32_t yesterday_contribution;
    uint32_t lifetime_honorbale_kills;
    uint32_t byte_s2;
    uint32_t watched_faction_index;
    uint32_t combat_rating1[25];
    uint32_t arena_team_info11[21];
    uint32_t honor_currency;
    uint32_t arena_currency;
    uint32_t max_level;
    uint32_t daily_quests1[25];
    float rune_regen1[4];
    uint32_t no_reagent_cost1[3];
    uint32_t glyph_slots1[6];
    uint32_t glyphs1[6];
    uint32_t glyphs_enabled;
    uint32_t pet_spell_power;
};

struct UnitEntry {
    guid_t charm;
    guid_t summon;
    guid_t critter;
    guid_t charmed_by;
    guid_t summoned_by;
    guid_t created_by;
    guid_t target;
    guid_t channel_object;
    uint32_t channel_spell;
    uint8_t bytes0[4];
    uint32_t health;
    uint32_t power[7];
    uint32_t maxhealth;
    uint32_t maxpower[7];
    float power_regen_flat_modifier[7];
    float power_regen_interrupted_flat_modifier[7];
    uint32_t level;
    uint32_t factiontemplate;
    uint32_t virtual_item_slot_id[3];
    uint32_t flags;
    uint32_t flags2;
    uint32_t aurastate;
    uint32_t baseattacktime[2];
    uint32_t rangedattacktime;
    float boundingradius;
    float combatreach;
    uint32_t displayid;
    uint32_t nativedisplayid;
    uint32_t mountdisplayid;
    float mindamage;
    float maxdamage;
    float minoffhanddamage;
    float maxoffhanddamage;
    uint8_t bytes1[4];
    uint32_t petnumber;
    uint32_t pet_name_timestamp;
    uint32_t petexperience;
    uint32_t petnextlevelexp;
    uint32_t dynamic_flags;
    float mod_cast_speed;
    uint32_t created_by_spell;
    NPCFlags npc_flags;
    uint32_t npc_emotestate;
    uint32_t stat[5];
    uint32_t posstat[5];
    uint32_t negstat[5];
    uint32_t resistances[7];
    uint32_t resistancebuffmodspositive[7];
    uint32_t resistancebuffmodsnegative[7];
    uint32_t base_mana;
    uint32_t base_health;
    uint8_t bytes2[4];
    uint32_t attack_power;
    uint16_t attack_power_mods[2];
    float attack_power_multiplier;
    uint32_t ranged_attack_power;
    uint16_t ranged_attack_power_mods[2];
    float ranged_attack_power_multiplier;
    float minrangeddamage;
    float maxrangeddamage;
    uint32_t power_cost_modifier[7];
    float power_cost_multiplier[7];
    float maxhealthmodifier;
    float hoverheight;
    uint32_t pad;

    PlayerEntry player_entry;
};

struct ItemEntry {
    guid_t owner;
    guid_t contained;
    guid_t creator;
    guid_t giftcreator;
    uint32_t stack_count;
    uint32_t duration;
    uint32_t spell_charges[5];
    uint32_t flags;
    uint32_t enchantment11[2];
    uint16_t enchantment13[2];
    uint32_t enchantment21[2];
    uint16_t enchantment23[2];
    uint32_t enchantment31[2];
    uint16_t enchantment33[2];
    uint32_t enchantment41[2];
    uint16_t enchantment43[2];
    uint32_t enchantment51[2];
    uint16_t enchantment53[2];
    uint32_t enchantment61[2];
    uint16_t enchantment63[2];
    uint32_t enchantment71[2];
    uint16_t enchantment73[2];
    uint32_t enchantment81[2];
    uint16_t enchantment83[2];
    uint32_t enchantment91[2];
    uint16_t enchantment93[2];
    uint32_t enchantment101[2];
    uint16_t enchantment103[2];
    uint32_t enchantment111[2];
    uint16_t enchantment113[2];
    uint32_t enchantment121[2];
    uint16_t enchantment123[2];
    uint32_t property_seed;
    uint32_t random_properties_id;
    uint32_t durability;
    uint32_t maxdurability;
    uint32_t create_played_time;
    uint32_t pad;
};

struct ContainerEntry {
    uint32_t num_slots;
    uint32_t _alignment;
    uint64_t slot1[36];
};

static_assert(sizeof(ObjectEntry) == 0x18);
static_assert(sizeof(GameObjectEntry) == 0x30);
static_assert(sizeof(DynamicObjectEntry) == 0x18);
static_assert(sizeof(CorpseEntry) == 0x78);
static_assert(sizeof(PlayerEntry) == 0x1268);
static_assert(sizeof(UnitEntry) == 0x14A0);
static_assert(sizeof(ItemEntry) == 0xE8);
static_assert(sizeof(ContainerEntry) == 0x128);
