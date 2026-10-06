#pragma once

#include "BaseTypes.h"

#include "Graphics/CGxDevice.h"
#include "M2/M2Data.h"
#include "M2/M2Sequence.h"
#include "Lib/Storm.h"
#include "Texture/CTexture.h"

class CM2Cache;

class CM2Shared {
public:
    uint32_t ref_count_;
    CM2Cache* cache_;

    uint32_t flags_;

    CAsyncObject* asc_obj_;
    SlotList<CM2Model> callback_list_;
    TSExplicitList<CM2SequenceLoad> seq_load_list_;
    uint32_t low_prio_seq_count_;
    CM2SequenceLoad** low_prio_sequences_;
    uint32_t low_prio_seq_load_slot_index_;
    SlotLink<CM2Shared> free_link_;
    uint32_t freed_timestamp_;
    char file_path_[260];
    char* basename_;
    SlotLink<CM2Shared> hash_link_;
    uint32_t basename_hash_;
    M2Data* data_;
    AaBox aa_box_;
    uint32_t size_;
    M2SkinProfile* skin_profile_;
    CTexture** textures_;

    CGxPool* index_pool;
    CGxBuf* index_buf;
    CGxPool* vertex_pool;
    CGxBuf* vertex_buf;
    void** batch_shaders;

    M2SkinSection* skin_sections_;

    uint32_t _uint190;
    uint32_t _uint194;
    uint16_t force_full_animate_;
    char _pad[2];
    uint32_t last_update_time_;
    TSLink<CM2Shared> update_list_link_;
};

static_assert(sizeof(CM2Shared) == 0x1A8);
