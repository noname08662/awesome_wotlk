#pragma once

class CM2Cache {
public:
    int32_t initialized_;
    uint32_t flags_;
    SlotList<CM2Shared> free_list_;
    CM2Shared* buckets_[1021];
    HANDLE thread_handle_;
    uint32_t wake_event_;
    uint32_t done_event_;
    uint32_t tls_value_;
    void (*thread_proc_)(void* param);
    void* thread_param_;
    unk_t _unk1004[32];
    uint32_t last_update_time_;
    TSExplicitList<void> update_list_;
};

static_assert(sizeof(CM2Cache) == 0x10AC);
