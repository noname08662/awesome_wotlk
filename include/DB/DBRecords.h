#pragma once

#include <cstdint>

#include "BaseTypes.h"

#include "Character/CharacterEnums.h"
#include "ObjectManager/ObjectManagerEnums.h"

struct CharSectionsRec {
    uint32_t id;
    uint32_t race_id;
    CharGender sex_id;
    CharSectionType base_section;
    const char* texture_name[3];
    CharSectionFlags flags;
    uint32_t variation_index;
    uint32_t color_index;
};

struct ComponentCharData {
    RaceId race_id;
    CharGender gender_id;
    ClassId class_id;
    uint32_t hair_color;
    uint32_t skin_id;
    uint32_t face_id;
    uint32_t facial_hair_style;
    uint32_t hair_style;
};

struct LockRec {
    enum Type : int {
        eLockKeyNone = 0,
        eLockKeyItem = 1,
        eLockKeySkill = 2,  // profession skill check (e.g. gathering nodes)
    };

    int id;
    Type type[8];
    int index[8];
    int skill[8];
    int action[8];
};

struct ItemCacheRec {
    uint32_t id;
    uint32_t item_class;
    uint32_t sub_class;
    uint32_t unk_int;
    uint32_t display_id;
    uint32_t quality;
    int flags_and_faction[2];
    int buy_price;
    int sell_price;
    int inv_type;
    int allow_class;
    int allow_race;
    int item_lvl;
    int req_lvl;
    int req_skill;
    int req_skill_rank;
    int req_spell;
    int req_honor;
    int req_city_rank;
    int req_rep_faction;
    int req_rep_rank;
    int max_count;
    int stackable;
    int container_slots;
    int stats_count;
    int stats[10][2];
    int scaling_stat_distribution;
    int scaling_stat_value;
    float sd_dmg1[2];
    float sd_dmg2[2];
    int sd_dmg_type[2];
    int resistance[7];
    int delay;
    int ammo_type;
    float ranged_mod_range;
    int spell_id[5];
    int spell_trigger[5];
    int spell_charges[5];
    int spell_cooldown[5];
    int spell_category[5];
    int spell_cat_cooldown[5];
    int bonding;
    char* description;
    int page_text_id;
    int language_id;
    int page_material;
    int start_quest;
    int lock_id;
    int material;
    int sheath;
    int random_property;
    int random_suffix;
    int block;
    int item_set_id;
    int max_durability;
    int area;
    int map;
    int bag_family;
    int totem_category;
    int socket_color[3];
    int socket_item[3];
    int socket_bonus;
    int gem_properties;
    int req_disenchant_skill;
    float armor_dmg_mod;
    int duration;
    int item_limit_cat;
    int holiday;
    char name[4][400];
};

static_assert(sizeof(ItemCacheRec) == 0x834);

struct ItemClassRec {
    uint32_t class_id;
    uint32_t subclass_map_id;
    uint32_t flags;
    char* class_name_lang;
};

struct ItemSubClassRec {
    uint32_t class_id;
    uint32_t sub_class_id;
    uint32_t prerequisite_proficiency;
    uint32_t postrequisite_proficiency;
    uint32_t flags;
    uint32_t display_flags;
    uint32_t weapon_parry_seq;
    uint32_t weapon_ready_seq;
    uint32_t weapon_attack_seq;
    uint32_t weapon_swing_size;
    char* display_name_lang;
    char* verbose_name_lang;
};

struct ItemDisplayInfoRec {
    uint32_t id;
    char* model_name[2];
    char* model_texture[2];
    char* inventory_icon;
    uint32_t ground_model;
    uint32_t geoset_group[3];
    uint32_t spell_visual_id;
    uint32_t group_sound_index;
    uint32_t helmet_geoset_vis_id[2];
    uint32_t texture[8];
    uint32_t item_visual;
};

struct SpellRec {
    using HasShapeshiftFlagT = bool (*)(SpellRec*);
    inline static auto has_shapeshift_flag_fn = reinterpret_cast<HasShapeshiftFlagT>(0x00800950);

    bool hasShapeshiftFlag() { return has_shapeshift_flag_fn(this); }

    uint32_t id;
    uint32_t category;
    uint32_t dispel;
    int32_t mechanic;

    uint32_t attributes;
    uint32_t attributes_ex;
    uint32_t attributes_ex2;
    uint32_t attributes_ex3;
    uint32_t attributes_ex4;
    uint32_t attributes_ex5;
    uint32_t attributes_ex6;
    uint32_t attributes_ex7;

    uint32_t stances;
    unk_t _unk_34;
    uint32_t stances_not;
    unk_t _unk_3C;
    uint32_t targets;
    uint32_t target_creature_type;
    uint32_t requires_spell_focus;
    uint32_t facing_caster_flags;
    uint32_t caster_aura_state;
    uint32_t target_aura_state;
    uint32_t caster_aura_state_not;
    uint32_t target_aura_state_not;
    uint32_t caster_aura_spell;
    uint32_t target_aura_spell;
    uint32_t exclude_caster_aura_spell;
    uint32_t exclude_target_aura_spell;
    uint32_t casting_time_index;
    uint32_t recovery_time;
    uint32_t category_recovery_time;
    uint32_t interrupt_flags;
    uint32_t aura_interrupt_flags;
    uint32_t channel_interrupt_flags;
    uint32_t proc_flags;
    uint32_t proc_chance;
    uint32_t proc_charges;
    uint32_t max_level;
    uint32_t base_level;
    uint32_t spell_level;
    uint32_t duration_index;
    int32_t power_type;
    uint32_t mana_cost;
    uint32_t mana_cost_perlevel;
    uint32_t mana_per_second;
    uint32_t mana_per_second_per_level;
    uint32_t range_index;
    float speed;
    uint32_t modal_next_spell;
    uint32_t stack_amount;
    uint32_t totem[2];
    int32_t reagent[8];
    uint32_t reagent_count[8];
    int32_t equipped_item_class;
    int32_t equipped_item_sub_class_mask;
    int32_t equipped_item_inventory_type_mask;

    int32_t effect[3];
    int32_t effect_die_sides[3];
    float effect_real_points_per_level[3];
    int32_t effect_base_points[3];
    uint32_t effect_mechanic[3];
    uint32_t effect_implicit_target_a[3];
    uint32_t effect_implicit_target_b[3];
    uint32_t effect_radius_index[3];
    uint32_t effect_apply_aura_name[3];
    uint32_t effect_amplitude[3];
    float effect_multiple_value[3];
    uint32_t effect_chain_target[3];
    uint32_t effect_item_type[3];
    int32_t effect_misc_value[3];
    int32_t effect_misc_value_b[3];
    uint32_t effect_trigger_spell[3];
    float effect_points_per_combo_point[3];
    Flag96 effect_spell_class_mask[3];

    uint32_t spell_visual[2];
    uint32_t spell_icon_id;
    uint32_t active_icon_id;
    uint32_t spell_priority;

    uint32_t spell_name_offset;  // string block
    uint32_t rank_offset;
    uint32_t description_offset;
    uint32_t tool_tip_offset;

    uint32_t mana_cost_percentage;
    uint32_t start_recovery_category;
    uint32_t start_recovery_time;
    uint32_t max_target_level;
    uint32_t spell_family_name;
    Flag96 spell_family_flags;
    uint32_t max_affected_targets;
    uint32_t dmg_class;
    uint32_t prevention_type;
    uint32_t stance_bar_order;

    float dmg_multiplier[3];
    uint32_t min_faction_id;
    uint32_t min_reputation;
    uint32_t required_aura_vision;
    uint32_t totem_category[2];
    int32_t area_group_id;
    int32_t school_mask;
    uint32_t rune_cost_id;
    uint32_t spell_missile_id;
    uint32_t power_display_id;
    unk_t _unk_294[3];
    uint32_t spell_description_variable_id;
    uint32_t spell_difficulty_id;
};

static_assert(sizeof(SpellRec) == 0x2A8);

struct CreatureCache {
    uint32_t id;
    char* sub_name_p;
    char* icon_name_p;
    uint32_t type_flags;
    TypeMask type;
    uint32_t family;
    uint32_t rank;
    int kill_credit[2];
    int display_id[4];
    float hp_modifier;
    float mp_modifier;
    char racial_leader[4];
    int quest_item[6];
    int movement_id;
    char name[4][1024];
    char sub_name[1024];
    char icon_name[1024];
};

static_assert(sizeof(CreatureCache) == 0x185C);

struct FactionTemplateRec {
    uint32_t id;
    uint32_t faction;
    uint32_t flags;
    uint32_t faction_group;
    uint32_t friend_group;
    uint32_t enemy_group;
    uint32_t enemies[4];
    uint32_t friends[4];
};

static_assert(sizeof(FactionTemplateRec) == 0x38);

struct CreatureDisplayInfoRec {
    uint32_t id;
    uint32_t model_id;
    uint32_t sound_id;
    uint32_t extended_display_info_id;
    float creature_model_scale;
    uint32_t creature_model_alpha;
    char* texture_variation[3];
    char* portrait_texture_name;
    uint32_t size_class;
    uint32_t blood_id;
    uint32_t npc_sound_id;
    uint32_t particle_color_id;
    uint32_t creature_geoset_data;
    uint32_t object_effect_package_id;
    uint32_t anim_replacement_set_id;
};

static_assert(sizeof(CreatureDisplayInfoRec) == 0x44);

struct CreatureDisplayInfoExtraRec {
    uint32_t id;
    uint32_t display_race_id;
    uint32_t display_sex_id;
    uint32_t skin_id;
    uint32_t face_id;
    uint32_t hair_style_id;
    uint32_t hair_color_id;
    uint32_t facial_hair_id;
    uint32_t npc_item_display[11];
    uint32_t flags;
    char* bake_name;
};

static_assert(sizeof(CreatureDisplayInfoExtraRec) == 0x54);

struct CreatureModelDataRec {
    uint32_t id;
    uint32_t flags;
    const char* model_name;
    uint32_t size_class;
    float model_scale;
    uint32_t blood_id;
    uint32_t footprint_texture_id;
    float footprint_texture_length;
    float footprint_texture_width;
    float footprint_particle_scale;
    uint32_t foley_material_id;
    uint32_t footstep_shake_size;
    uint32_t death_thud_shake_size;
    uint32_t sound_id;
    float collision_width;
    float collision_height;
    float mount_height;
    float geo_box_min_x;
    float geo_box_min_y;
    float geo_box_min_z;
    float geo_box_max_x;
    float geo_box_max_y;
    float geo_box_max_z;
    float world_effect_scale;
    float attached_effect_scale;
    float missile_collision_radius;
    float missile_collision_push;
    float missile_collision_raise;
};

static_assert(sizeof(CreatureModelDataRec) == 0x70);

struct CreatureSoundDataRec {
    uint32_t id;
    uint32_t sound_exertion_id;
    uint32_t sound_exertion_critical_id;
    uint32_t sound_injury_id;
    uint32_t sound_injury_critical_id;
    uint32_t sound_injury_crushing_blow_id;
    uint32_t sound_death_id;
    uint32_t sound_stun_id;
    uint32_t sound_stand_id;
    uint32_t sound_footstep_id;
    uint32_t sound_aggro_id;
    uint32_t sound_wing_flap_id;
    uint32_t sound_wing_glide_id;
    uint32_t sound_alert_id;
    uint32_t sound_fidget[5];
    uint32_t custom_attack[4];
    uint32_t npc_sound_id;
    uint32_t loop_sound_id;
    uint32_t creature_impact_type;
    uint32_t sound_jump_start_id;
    uint32_t sound_jump_end_id;
    uint32_t sound_pet_attack_id;
    uint32_t sound_pet_order_id;
    uint32_t sound_pet_dismiss_id;
    float fidget_delay_seconds_min;
    float fidget_delay_seconds_max;
    uint32_t birth_sound_id;
    uint32_t spell_cast_directed_sound_id;
    uint32_t submerge_sound_id;
    uint32_t submerged_sound_id;
    uint32_t creature_sound_data_id_pet;
    uint32_t transform_sound_id;
    uint32_t transform_animated_sound_id;
};

static_assert(sizeof(CreatureSoundDataRec) == 0xA0);

struct UnitBloodLevelsRec {
    uint32_t id;
    uint32_t violence_level[3];
};

static_assert(sizeof(UnitBloodLevelsRec) == 0x10);
