#ifndef lmemPool_h
#define lmemPool_h

#include <cstddef>
#include <cstdint>

#include "../src/lua.h"
#include "include/Lib/Storm.h"

class MemChunk {
public:
    void* m_memblocks;
    void* m_free;
    uint32_t m_size;
    uint32_t m_blockSize;
    uint32_t m_numBlocks;

    MemChunk(uint32_t blockSize, uint32_t size);
    MemChunk(const MemChunk&) = delete;
    MemChunk(MemChunk&&) = delete;
    MemChunk& operator=(const MemChunk&) = delete;
    MemChunk& operator=(MemChunk&&) = delete;
    ~MemChunk();

    void operator delete(void* ptr) { storm::memory::free::viaDetour(ptr, __FILE__, __LINE__, 0); }
};

class MemPool {
public:
    TSBaseArray<MemChunk*> m_chunks{};
    uint32_t m_chunkSize;
    uint32_t m_blockSize;
    uint32_t m_numBlocks;
    uint32_t m_firstFree = 0;
    struct MemChunkSpan* m_spans = nullptr;

    explicit MemPool(uint32_t blockSize);
    MemPool(const MemPool&) = delete;
    MemPool(MemPool&&) = delete;
    MemPool& operator=(const MemPool&) = delete;
    MemPool& operator=(MemPool&&) = delete;
    ~MemPool();

    void operator delete(void* ptr) { storm::memory::free::viaDetour(ptr, __FILE__, __LINE__, 0); }

    void* MemAlloc();
    int32_t MemFree(void* ptr);
};

static_assert(offsetof(MemPool, m_firstFree) == 0x18, "MemPool keeps Wow.exe's first 0x18 bytes");

LUAI_FUNC void* luaM_initPool();
LUAI_FUNC void luaM_freePool(void* ud);
LUAI_FUNC void* luaM_reallocPool(void* ud, void* ptr, size_t osize, size_t nsize);

#endif
