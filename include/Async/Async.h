#pragma once

#include <cstdint>

#include "Lib/Storm.h"

class CAsyncObject;
class CAsyncQueue;

class CAsyncThread {
public:
    inline static auto* TSL = reinterpret_cast<::TSExplicitList<CAsyncThread>*>(0x00AC3454);

    TSLink<CAsyncThread> link_;
    HANDLE thread_;
    CAsyncQueue* queue_;
    CAsyncObject* current_object_;
};

static_assert(sizeof(CAsyncThread) == 0x14);

class CAsyncQueue {
public:
    inline static auto* TSL = reinterpret_cast<TSExplicitList<CAsyncQueue>*>(0x00AC3448);

    TSLink<CAsyncQueue> link_;
    TSExplicitList<CAsyncObject> active_list_;
    TSExplicitList<CAsyncObject> waiting_list_;
    uint32_t is_priority_queue_;
};

static_assert(sizeof(CAsyncQueue) == 0x24);

class CAsyncObject {
public:
    enum Priority : uint8_t {
        eAsyncStateIdle = 0x7E,
        eAsyncPriorityDefault = 0x80,
        eAsyncPriorityLow = 0x81,
        eAsyncPriorityNormal = 0x82,
        eAsyncPriorityHigh = 0x83,
    };

    inline static auto* TSL_free = reinterpret_cast<TSExplicitList<CAsyncQueue>*>(0x00AC3460);
    inline static auto* TSL_completed = reinterpret_cast<TSExplicitList<CAsyncQueue>*>(0x00AC346C);

    using AsyncWorker = int32_t(__cdecl*)(CAsyncObject* async_obj);
    using AsyncComplete = void(__cdecl*)(CAsyncObject* async_obj);

    TSLink<CAsyncObject> link_free_;
    uint32_t size_;
    void* owner_;
    AsyncComplete complete_cb_;
    AsyncWorker worker_cb_;
    CAsyncQueue* queue_;
    uint32_t tick_count_;
    Priority priority_;
    uint8_t flags_;
    uint8_t is_read_complete_;
    uint8_t in_progress_;
    uint8_t is_remote_;
    uint8_t is_queued_;
    char _pad[2];
    TSLink<CAsyncObject> link_;
};

static_assert(sizeof(CAsyncObject) == 0x30);
