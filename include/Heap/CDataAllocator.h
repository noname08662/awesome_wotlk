#pragma once

#include <hookkit/hook.h>

#include <cstdint>

template <typename T>
union CDataNode {
    CDataNode<T>* next_free;
    T obj;
};

// one MemAlloc(block_size * blocks_per_chunk + 4), blocks follow the link
template <typename T>
struct CDataChunk {
    CDataChunk<T>* next_chunk;

    CDataNode<T>* data() { return reinterpret_cast<CDataNode<T>*>(this + 1); }

    const CDataNode<T>* data() const { return reinterpret_cast<const CDataNode<T>*>(this + 1); }
};

template <typename T>
struct CDataAllocator {
    uint32_t block_size;
    uint32_t blocks_per_chunk;
    uint32_t active_count;
    CDataChunk<T>* chunk_list;
    CDataNode<T>* free_list;

    // chunks are never released before wipe, so this is a plain walk
    [[nodiscard]]
    const CDataChunk<T>* chunkOf(const void* ptr) const {
        const auto addr = reinterpret_cast<uintptr_t>(ptr);
        const uintptr_t span = uintptr_t{block_size} * blocks_per_chunk;
        for (const CDataChunk<T>* cur = chunk_list; cur != nullptr; cur = cur->next_chunk) {
            if (addr - reinterpret_cast<uintptr_t>(cur->data()) < span) { return cur; }
        }
        return nullptr;
    }

    // every block of every chunk, live or free; a free block holds next_free where T starts.
    // GetData links new chunks in at the head, so blocks allocated from inside fn can't invalidate the walk
    template <typename Fn>
    void forEachBlock(Fn&& fn) {
        for (CDataChunk<T>* cur = chunk_list; cur != nullptr; cur = cur->next_chunk) {
            const auto base = reinterpret_cast<uintptr_t>(cur->data());
            for (uint32_t i = 0; i < blocks_per_chunk; ++i) {
                fn(reinterpret_cast<T*>(base + uintptr_t{i} * block_size));
            }
        }
    }

    // ptr is the start of one of this allocator's blocks, live or free
    [[nodiscard]]
    bool owns(const T* ptr) const {
        const CDataChunk<T>* chunk = chunkOf(ptr);
        return chunk != nullptr &&
            (reinterpret_cast<uintptr_t>(ptr) - reinterpret_cast<uintptr_t>(chunk->data())) % block_size == 0;
    }

    HOOKKIT_HOOK(
        ctor, 0x0095D080, hookkit::Conv::eThiscall, CDataAllocator<T>*, CDataAllocator<T>*, uint32_t, uint32_t);
    // zero init, file, line
    HOOKKIT_HOOK(get, 0x0095D110, hookkit::Conv::eThiscall, T*, CDataAllocator<T>*, int, const char*, int32_t);
    HOOKKIT_HOOK(free, 0x0095D1B0, hookkit::Conv::eThiscall, void, CDataAllocator<T>*, T*);
    HOOKKIT_HOOK(wipe, 0x0095D1D0, hookkit::Conv::eThiscall, uint32_t, CDataAllocator<T>*);
};
