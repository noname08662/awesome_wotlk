#include "lmemPool.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <span>

struct MemChunkSpan {
    uintptr_t begin;
    uintptr_t end;
    uint32_t index;
};

namespace {
constexpr std::array<uint32_t, 9> s_blocksizes = {16, 24, 32, 40, 64, 80, 128, 160, 256};

// luaM_initPool's MemPool*[], one per size class
using PoolArray = std::span<MemPool*, s_blocksizes.size()>;

MemPool* poolfor(PoolArray pools, size_t size) {
    for (size_t i = 0; i < pools.size(); i++) {
        if (size <= s_blocksizes[i]) { return pools[i]; }
    }

    return nullptr;
}

uint32_t findchunk(const MemPool* pool, const void* ptr) {
    const std::span<const MemChunkSpan> spans{pool->m_spans, pool->m_chunks.size()};
    const auto p = reinterpret_cast<uintptr_t>(ptr);
    const auto after = std::ranges::upper_bound(spans, p, {}, &MemChunkSpan::begin);

    if (after == spans.begin() || p >= std::prev(after)->end) { return pool->m_chunks.size(); }

    return std::prev(after)->index;
}

void addspan(MemPool* pool, uint32_t index) {
    const MemChunk* chunk = pool->m_chunks[index];
    const auto begin = reinterpret_cast<uintptr_t>(chunk->m_memblocks);
    pool->m_spans = static_cast<MemChunkSpan*>(
        storm::memory::reAlloc::viaDetour(pool->m_spans, sizeof(MemChunkSpan) * (index + 1), __FILE__, __LINE__, 0));

    const std::span spans{pool->m_spans, index + 1};
    uint32_t at = index;

    for (; at > 0 && spans[at - 1].begin > begin; at--) {
        spans[at] = spans[at - 1];
    }

    spans[at] = MemChunkSpan{.begin = begin, .end = begin + chunk->m_size, .index = index};
}

void setchunkcount(TSBaseArray<MemChunk*>& chunks, uint32_t count) {
    if (count != chunks.count && (chunks.data != nullptr || count != 0)) {
        chunks.capacity = count;
        chunks.data = static_cast<MemChunk**>(
            storm::memory::reAlloc::viaDetour(chunks.data, sizeof(MemChunk*) * count, __FILE__, __LINE__, 0));
    }

    chunks.count = count;
}
}  // namespace

MemChunk::MemChunk(uint32_t blockSize, uint32_t size)
    : m_memblocks(nullptr),
      m_free(nullptr),
      m_size{size / blockSize * blockSize},
      m_blockSize{blockSize},
      m_numBlocks{size / blockSize} {
    this->m_memblocks = storm::memory::alloc::viaDetour(this->m_size, __FILE__, __LINE__, 0);
    this->m_free = this->m_memblocks;

    const std::span blocks{static_cast<std::byte*>(this->m_memblocks), this->m_size};
    uint32_t at = 0;

    for (; at + this->m_blockSize < this->m_size; at += this->m_blockSize) {
        *reinterpret_cast<void**>(&blocks[at]) = &blocks[at + this->m_blockSize];
    }

    *reinterpret_cast<void**>(&blocks[at]) = nullptr;
}

MemChunk::~MemChunk() { storm::memory::free::viaDetour(this->m_memblocks, __FILE__, __LINE__, 0); }

MemPool::MemPool(uint32_t blockSize) : m_chunkSize{blockSize * 1024}, m_blockSize{blockSize}, m_numBlocks{1024} {}

MemPool::~MemPool() {
    for (uint32_t i = this->m_chunks.size(); i > 0; i--) {
        delete this->m_chunks[i - 1];
    }

    if (this->m_spans != nullptr) { storm::memory::free::viaDetour(this->m_spans, __FILE__, __LINE__, 0); }

    if (this->m_chunks.data != nullptr) { storm::memory::free::viaDetour(this->m_chunks.data, __FILE__, __LINE__, 0); }
}

void* MemPool::MemAlloc() {
    const uint32_t count = this->m_chunks.size();
    uint32_t i = this->m_firstFree;

    while (i < count && this->m_chunks[i]->m_free == nullptr) {
        i++;
    }

    if (i == count) {
        setchunkcount(this->m_chunks, count + 1);
        this->m_chunks[count] = new (__FILE__, __LINE__) MemChunk(this->m_blockSize, this->m_chunkSize);
        addspan(this, count);
    }

    this->m_firstFree = i;

    MemChunk* chunk = this->m_chunks[i];
    void* block = chunk->m_free;

    if (block != nullptr) {
        chunk->m_numBlocks--;
        chunk->m_free = *static_cast<void**>(block);
    }

    return block;
}

int32_t MemPool::MemFree(void* ptr) {
    const uint32_t i = findchunk(this, ptr);

    if (i == this->m_chunks.size()) { return 0; }

    MemChunk* chunk = this->m_chunks[i];
    *static_cast<void**>(ptr) = chunk->m_free;
    chunk->m_numBlocks++;
    chunk->m_free = ptr;
    this->m_firstFree = std::min(this->m_firstFree, i);

    return 1;
}

void* luaM_initPool() {
    const PoolArray pools{new (__FILE__, __LINE__) MemPool*[s_blocksizes.size()], s_blocksizes.size()};

    for (size_t i = 0; i < pools.size(); i++) {
        pools[i] = new (__FILE__, __LINE__) MemPool(s_blocksizes[i]);
    }

    return pools.data();
}

void luaM_freePool(void* ud) {
    if (ud == nullptr) { return; }

    for (MemPool* pool : PoolArray{static_cast<MemPool**>(ud), s_blocksizes.size()}) {
        delete pool;
    }

    storm::memory::free::viaDetour(ud, __FILE__, __LINE__, 0);
}

void* luaM_reallocPool(void* ud, void* ptr, size_t osize, size_t nsize) {
    const PoolArray pools{static_cast<MemPool**>(ud), s_blocksizes.size()};
    MemPool* const oldpool = ptr != nullptr ? poolfor(pools, osize) : nullptr;

    if (nsize == 0) {
        if (oldpool != nullptr) {
            oldpool->MemFree(ptr);
        } else if (ptr != nullptr) {
            storm::memory::free::viaDetour(ptr, __FILE__, __LINE__, 0);
        }

        return nullptr;
    }

    MemPool* const newpool = poolfor(pools, nsize);

    if (oldpool == newpool) {
        return oldpool == nullptr ? storm::memory::reAlloc::viaDetour(ptr, nsize, __FILE__, __LINE__, 0) : ptr;
    }

    void* newptr =
        newpool == nullptr ? storm::memory::alloc::viaDetour(nsize, __FILE__, __LINE__, 0) : newpool->MemAlloc();

    if (ptr != nullptr) {
        std::memcpy(newptr, ptr, std::min(osize, nsize));

        if (oldpool != nullptr) {
            oldpool->MemFree(ptr);
        } else {
            storm::memory::free::viaDetour(ptr, __FILE__, __LINE__, 0);
        }
    }

    return newptr;
}
