#pragma once

class CM2Shared;
class CM2Model;

struct CM2BoneSequenceState {
    uint32_t key_time;
    uint32_t key_variant_index;
    uint16_t variation_id;
    uint8_t ended;
    uint8_t auto_pick_variation;
    int32_t start_time;
    int32_t end_time;
    float rate;
    float inv_rate;
    int32_t blend_length;
    int32_t chosen_variant_index;
};

static_assert(sizeof(CM2BoneSequenceState) == 0x24);

struct CM2SequencePlayBack {
    enum Flags : uint16_t {
        eHasCachedM2Data = 0x1,
        eIsPrimarySequence = 0x2,
        eUsedDefaultRate = 0x4,
    };

    TSLink<CM2SequencePlayBack> link;
    CM2Model* model;
    uint16_t bone_index;
    Flags flags;
    float rate;
    float inv_rate;
    uint32_t next_sequence_id;
};

static_assert(sizeof(CM2SequencePlayBack) == 0x1C);

struct CM2SequenceLoad {
    TSLink<CM2SequenceLoad> link;
    CAsyncObject* asc_obj;
    CM2Shared* shared;
    uint16_t requested_seq_id;
    uint16_t load_slot_index;
    TSExplicitList<CM2SequencePlayBack> seq_pb_list;
};

static_assert(sizeof(CM2SequenceLoad) == 0x20);
