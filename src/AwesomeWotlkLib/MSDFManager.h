#pragma once

#include <ankerl/unordered_dense.h>
#include <hookkit/accessor.h>
#include <windows.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "MSDFCache.h"
#include "MSDFUtils.h"

class MSDFManager {
    friend class MSDFCache;
    friend class MSDFPregen;

public:
    MSDFManager() = delete;
    ~MSDFManager() = delete;
    MSDFManager(const MSDFManager&) = delete;
    MSDFManager& operator=(const MSDFManager&) = delete;
    MSDFManager(MSDFManager&&) = delete;
    MSDFManager& operator=(MSDFManager&&) = delete;

private:
    static constexpr size_t kMaxArenaSlots = 16;
    static constexpr uint32_t kInvalidIndex = 0xFFFFFFFF;
    static_assert(kMaxArenaSlots <= 64);
    static_assert(MSDFCache::kBlockSize > 0 && (MSDFCache::kBlockSize & (MSDFCache::kBlockSize - 1)) == 0,
        "kBlockSize must be a power of 2");

    struct alignas(128) MappedBlock {
        FileGuard file;
        MappingGuard mapping;
        ViewGuard view;
        uint64_t file_size = 0;
        const MSDFCache::BlockFileHeader* header = nullptr;
        std::span<const MSDFCache::GlyphEntry> entries;
        std::span<const uint32_t> hash_table;
        std::span<const uint8_t> payload;
        uint32_t slot_index = kInvalidIndex;
        MSDFCache::BlockKey key;

        MappedBlock();
        ~MappedBlock() = default;
        MappedBlock(const MappedBlock&) = delete;
        MappedBlock& operator=(const MappedBlock&) = delete;
        MappedBlock(MappedBlock&&) = delete;
        MappedBlock& operator=(MappedBlock&&) = delete;

        void close();

        void reset() {
            close();
            slot_index = kInvalidIndex;
            key = {};
        }
    };

    static_assert(sizeof(MappedBlock) == 128);

    class ArenaState {
    public:
        ArenaState();
        ~ArenaState() = default;
        ArenaState(const ArenaState&) = delete;
        ArenaState& operator=(const ArenaState&) = delete;
        ArenaState(ArenaState&&) = delete;
        ArenaState& operator=(ArenaState&&) = delete;

        [[nodiscard]]
        bool isSlotOccupied(uint32_t i) const {
            return (free_mask_ & (1ULL << i)) == 0;
        }

        [[nodiscard]]
        bool isFull() const {
            return free_mask_ == 0;
        }

        [[nodiscard]]
        size_t slotSize() const {
            return effective_slot_size_;
        }

        [[nodiscard]]
        uint32_t slotBlockIndex(uint32_t slot_index) const {
            return slot_index < kMaxArenaSlots ? slot_to_block_index_[slot_index] : kInvalidIndex;
        }

        void* getFreeSlot(uint32_t block_index, uint32_t& out_slot_index);
        void freeSlot(uint32_t slot_index);
        void flushAll();

    private:
        size_t effective_slot_size_ = 0;
        uint64_t free_mask_ = (1ULL << kMaxArenaSlots) - 1;
        std::array<void*, kMaxArenaSlots> slot_addresses_{};
        std::array<uint32_t, kMaxArenaSlots> slot_to_block_index_{};
    };

    static bool loadGlyph(const MSDFCache::BlockWrap& wrap, uint32_t codepoint, GlyphMetrics& out_metrics);
    static void prefetchGlyphs(const MSDFCache::BlockWrap& wrap, std::span<const uint32_t> codepoints);

    static bool loadMappedBlock(
        const MSDFCache::BlockWrap& wrap, MappedBlock& out_block, void* slot_addr, uint32_t slot_index);
    static MappedBlock* getOrLoadMappedBlock(const MSDFCache::BlockWrap& wrap);

    static uint32_t getSlotBlockIndex(uint32_t slot_index) { return kArena->slotBlockIndex(slot_index); }

    static void freeBlock(uint32_t block_index);
    static void freeBlockByKey(MSDFCache::BlockKey key);
    static void flushAll();

    static uint32_t registerFont(FontHash hash);
    static FontHash getFontHash(uint32_t font_id);

    static SYSTEM_INFO querySystemInfo() {
        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        return si;
    }

    struct SystemInfoTag;
    static constexpr utils::Accessor<const SYSTEM_INFO, SystemInfoTag, &MSDFManager::querySystemInfo> kSystemInfo{};

    struct ArenaTag;
    static constexpr utils::Accessor<ArenaState, ArenaTag> kArena{};
    struct MappedBlocksTag;
    static constexpr utils::Accessor<std::array<MappedBlock, kMaxArenaSlots>, MappedBlocksTag> kMappedBlocks{};
    struct BlockCacheTag;
    static constexpr utils::Accessor<ankerl::unordered_dense::map<MSDFCache::BlockKey, uint32_t>, BlockCacheTag>
        kBlockCache{};
    struct LastBlockKeyTag;
    static constexpr utils::Accessor<MSDFCache::BlockKey, LastBlockKeyTag> kLastBlockKey{};

    struct FontHashToIdTag;
    static constexpr utils::Accessor<ankerl::unordered_dense::map<FontHash, uint32_t>, FontHashToIdTag> kFontHashToId{};
    struct FontIdToHashTag;
    static constexpr utils::Accessor<ankerl::unordered_dense::map<uint32_t, FontHash>, FontIdToHashTag> kFontIdToHash{};

    inline static uint32_t last_block_index_ = kInvalidIndex;
    inline static uint32_t next_font_id_ = 0;
};
