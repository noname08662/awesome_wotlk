#pragma once

#include <cstdint>

#include "Lib/Storm.h"

class CObjectHeapList {
public:
    struct CObjectHeap {
        void* data;
        uint32_t* free_list;
        uint32_t count;

        // size, capacity, tag
        HOOKKIT_HOOK(
            allocHeap, 0x004D2AA0, hookkit::Conv::eThiscall, void*, CObjectHeap*, size_t, uint32_t, const char*);
        // size, capacity, out slot, tag, out ptr, zero init
        HOOKKIT_HOOK(newObj, 0x004D2CC0, hookkit::Conv::eThiscall, int, CObjectHeap*, size_t, uint32_t, uint32_t*,
            const char*, void**, bool);
    };

    static_assert(sizeof(CObjectHeap) == 0xC);

    TSGrowableArray<CObjectHeap> heaps_;

    uint32_t object_size_;
    uint32_t capacity_per_heap_;
    uint32_t full_heaps_;
    uint32_t has_empty_heaps_;
    uint32_t auto_free_empty_heaps_;
    uint32_t current_heap_index_;
    char alloc_tag_[80];
    uint64_t total_allocations_;
    uint32_t _unused_80;
    uint8_t delay_free_empty_heaps_;
    char _pad[3];

    inline static auto* TSGRA = reinterpret_cast<TSGrowableArray<CObjectHeapList>*>(0x00B4AFC0);

    template <typename T>
    using Get = T* (*)(int, uint32_t);

    template <typename T>
    inline static Get<T> get_ = reinterpret_cast<Get<T>>(0x004D2D40);

    template <typename T>
    T* get(int category_id, uint32_t global_id) {
        return get_<T>(category_id, global_id);
    }

    // category id, global id
    HOOKKIT_HOOK(getRaw, 0x004D2D40, hookkit::Conv::eCdecl, void*, int, uint32_t);
    HOOKKIT_HOOK(freeEmptyHeaps, 0x004D2B30, hookkit::Conv::eThiscall, void, CObjectHeapList*);
    HOOKKIT_HOOK(getObjCount, 0x004D2C30, hookkit::Conv::eThiscall, uint32_t, CObjectHeapList*);
    HOOKKIT_HOOK(getHeapCount, 0x004D2C50, hookkit::Conv::eThiscall, uint32_t, CObjectHeapList*);
    HOOKKIT_HOOK(getPctUsed, 0x004D2C80, hookkit::Conv::eThiscall, uint32_t, CObjectHeapList*);
    // new capacity
    HOOKKIT_HOOK(reallocHeaps, 0x004D2E90, hookkit::Conv::eThiscall, CObjectHeap*, CObjectHeapList*, uint32_t);
    // obj idx
    HOOKKIT_HOOK(deleteObj, 0x004D2F00, hookkit::Conv::eThiscall, void, CObjectHeapList*, uint32_t);
    HOOKKIT_HOOK(clearHeaps, 0x004D3130, hookkit::Conv::eThiscall, void, CObjectHeapList*);
    // count, src heaps
    HOOKKIT_HOOK(transferHeaps, 0x004D3180, hookkit::Conv::eThiscall, void, CObjectHeapList*, uint32_t, CObjectHeap*);
    // out id, out ptr, zero init
    HOOKKIT_HOOK(newObj, 0x004D3250, hookkit::Conv::eThiscall, int, CObjectHeapList*, uint32_t*, void**, bool);
    // src
    HOOKKIT_HOOK(moveFrom, 0x004D3410, hookkit::Conv::eThiscall, CObjectHeapList*, CObjectHeapList*, CObjectHeapList*);

    template <typename T>
    T* newObjT(uint32_t* out_id, bool zero_init = false) {
        void* out_ptr = nullptr;
        if (newObj(out_id, &out_ptr, zero_init) != 0) { return static_cast<T*>(out_ptr); }
        return nullptr;
    }
};

static_assert(sizeof(CObjectHeapList) == 0x88);
