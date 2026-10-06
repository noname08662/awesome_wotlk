#pragma once

#include <cstdint>

class CMissile {
public:
    guid_t guid_;
    unk_t _unk0[4];
    CM2Model* model_;
    uint32_t spell_id_;
    unk_t _unk1[88];
};

static_assert(sizeof(CMissile) == 0x180);
