#pragma once
#include "MSDF.h"
#include "MSDFCache.h"
#include "ankerl/unordered_dense.h"
#include <filesystem>

class MSDFCache;

class MSDFManager {
    friend class MSDFCache;

public:
    MSDFManager();
    ~MSDFManager();
    MSDFManager(const MSDFManager&) = delete;
    MSDFManager& operator=(const MSDFManager&) = delete;
    MSDFManager(MSDFManager&&) = delete;
    MSDFManager& operator=(MSDFManager&&) = delete;

private:
    static constexpr size_t MAX_ARENA_SLOTS = 16;
    static_assert(MAX_ARENA_SLOTS <= 64);
    static_assert(MSDFCache::BLOCK_SIZE > 0 && (MSDFCache::BLOCK_SIZE & (MSDFCache::BLOCK_SIZE - 1)) == 0,
        "BLOCK_SIZE must be a power of 2");

    struct alignas(128) MappedBlock {
        FileGuard file;
        MappingGuard mapping;
        ViewGuard view;
        uint64_t fileSize = 0;
        const MSDFCache::BlockFileHeader* header = nullptr;
        const MSDFCache::GlyphEntry* entries = nullptr;
        const uint32_t* hashTable = nullptr;
        const uint8_t* payload = nullptr;
        uint32_t entryCount = 0;
        uint32_t slotIndex = 0xFFFFFFFF;
        MSDFCache::BlockKey key;

        void Close();
        void Reset() {
            Close();
            slotIndex = 0xFFFFFFFF;
            key = {};
        }

        MappedBlock() = default;
        MappedBlock(const MappedBlock&) = delete;
        MappedBlock& operator=(const MappedBlock&) = delete;
    };
    static_assert(sizeof(MappedBlock) == 128);

    struct ArenaState {
        void* base = nullptr;

        size_t effectiveSlotSize = 0;
        uint64_t freeMask = (1ULL << MAX_ARENA_SLOTS) - 1;

        std::array<void*, MAX_ARENA_SLOTS> slotAddresses;
        std::array<uint32_t, MAX_ARENA_SLOTS> slotToBlockIndex;

        bool IsSlotOccupied(uint32_t i) const { return !(freeMask & (1ULL << i)); }
        void* GetFreeSlot(uint32_t blockIndex, uint32_t& outSlotIndex);
        void FreeSlot(uint32_t slotIndex);
        void FlushAll();

        ArenaState();
        ~ArenaState();

        size_t SlotSize() const { return effectiveSlotSize; }
    };

    static bool LoadGlyph(const MSDFCache::BlockWrap& wrap, uint32_t codepoint, GlyphMetrics& outMetrics);

    static bool LoadMappedBlock(const MSDFCache::BlockWrap& wrap, MappedBlock& outBlock, void* slotAddr, uint32_t slotIndex);
    static MappedBlock* GetOrLoadMappedBlock(const MSDFCache::BlockWrap& wrap);

    static uint32_t GetSlotBlockIndex(uint32_t slotIndex) {
        return (slotIndex < MAX_ARENA_SLOTS) ? s_arena.slotToBlockIndex[slotIndex] : 0xFFFFFFFF;
    }

    static void FreeBlock(uint32_t blockIndex);
    static void FreeBlockByKey(MSDFCache::BlockKey key);
    static void FlushAll();

    static uint32_t RegisterFont(FontHash hash);
    static FontHash GetFontHash(uint32_t fontId);

    inline static std::array<MappedBlock, MAX_ARENA_SLOTS> s_mappedBlocks;
    inline static ankerl::unordered_dense::map<MSDFCache::BlockKey, uint32_t> s_blockCache;

    inline static uint32_t s_lastBlockIndex = 0xFFFFFFFF;
    inline static MSDFCache::BlockKey s_lastBlockKey;

    inline static ArenaState s_arena;
    inline static SYSTEM_INFO s_si;

    inline static ankerl::unordered_dense::map<FontHash, uint32_t> s_fontHashToId;
    inline static ankerl::unordered_dense::map<uint32_t, FontHash> s_fontIdToHash;

    inline static uint32_t s_nextFontId = 0;
};