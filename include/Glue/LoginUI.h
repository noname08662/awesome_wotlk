#pragma once

#include "BaseTypes.h"

#include "Lib/Storm.h"

class CCharacterComponent;
class CM2Model;

namespace login {
struct CharacterSelectionDisplay {
    inline static auto& TSGRA = *reinterpret_cast<TSGrowableArray<CharacterSelectionDisplay>*>(0x00B6B238);

    guid_t guid;
    char name[48];
    int map;
    int zone;
    int guild_id;
    Vec3f pos;
    int display_info_id[23];
    int inventory_type[23];
    int enchant_visual[23];
    int pet_display_id;
    int pet_level;
    int pet_family;
    int flags;
    int char_customize_flags;
    char race;
    char char_class;
    char gender;
    char skin;
    char face;
    char hair_style;
    char hair_color;
    char facial_color;
    char level;
    char first_login;
    unk_t _pad;
    CCharacterComponent* comp;
    CM2Model* pet_model;
    float field_190;
    unk_t _alignment;
};

static_assert(sizeof(CharacterSelectionDisplay) == 0x198);

inline void selectCharacter(int idx) {
    *reinterpret_cast<int*>(0x00AC436C) = idx;
    (reinterpret_cast<void (*)()>(0x004E3CD0))();
}

inline void enterWorld(int idx) {
    *reinterpret_cast<int*>(0x00B499A4) = *reinterpret_cast<int*>(0x00B1D618);
    *reinterpret_cast<int*>(0x00AC436C) = idx;
    (reinterpret_cast<void (*)()>(0x004D9BD0))();
}
}  // namespace login
