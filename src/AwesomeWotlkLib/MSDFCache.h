#pragma once
#include "MSDF.h"
#include <filesystem>
#include <atomic>
#include <deque>
#include <unordered_dense/include/ankerl/unordered_dense.h>

class MSDFFont;
class MSDFPregen;

class MSDFCache {
    friend class MSDFFont;
    friend class MSDFPregen;

private:
    struct MappedBlock;

public:
    MSDFCache(const FT_Byte* fontData, FT_Long dataSize,
        const char* familyName, const char* styleName,
        uint32_t sdfRenderSize, uint32_t sdfSpread);
    ~MSDFCache();

    MSDFCache(const MSDFCache&) = delete;
    MSDFCache& operator=(const MSDFCache&) = delete;
    MSDFCache(MSDFCache&&) = delete;
    MSDFCache& operator=(MSDFCache&&) = delete;

private:
    static constexpr const char* CACHE_DIR = "Cache_AwesomeWotLK";
    static constexpr uint32_t CACHE_VERSION = 1;
    static constexpr uint32_t BLOCK_MAGIC = 0x4D534442;
    static constexpr uint32_t MANIFEST_MAGIC = 0x4D534D46;
    static constexpr size_t WRITE_BATCH_SIZE = 128;
    static constexpr size_t BLOCK_SIZE = 512;
    static constexpr size_t TARGET_BLOCK_USAGE = 128 * 1024 * 1024;
    static constexpr size_t MAX_SAFE_ALLOCATION = 32 * 1024 * 1024;
    static constexpr size_t MAX_ARENA_SLOTS = 16;

#pragma pack(push, 1)
    struct ManifestHeader {
        uint32_t magic;
        uint32_t version;
        CacheKey key;
        uint32_t entryCount;
    };

    struct ManifestEntry {
        uint32_t codepoint;
        uint32_t blockId;
    };

    struct alignas(64) BlockFileHeader {
        uint32_t magic;
        uint32_t version;
        uint32_t blockId;
        uint32_t entryCount;
    };

    struct alignas(64) GlyphEntry {
        uint32_t codepoint;
        uint16_t width;
        uint16_t height;
        int16_t bitmapTop;
        int16_t bitmapLeft;
        uint32_t dataOffset;
        uint32_t dataSize;

        bool operator<(const GlyphEntry& other) const {
            return codepoint < other.codepoint;
        }
    };
#pragma pack(pop)

    static_assert(sizeof(ManifestHeader) == 24);
    static_assert(sizeof(ManifestEntry) == 8);
    static_assert(sizeof(BlockFileHeader) == 64);
    static_assert(sizeof(GlyphEntry) == 64);

    struct MappedBlock {
        FileGuard file;
        MappingGuard mapping;
        ViewGuard view;

        uint64_t fileSize = 0;
        const BlockFileHeader* header = nullptr;
        const GlyphEntry* entries = nullptr;
        const uint8_t* payload = nullptr;
        uint32_t entryCount = 0;

        bool isStale = false;
        std::filesystem::path stalePath;

        void Close();

        MappedBlock() = default;
        MappedBlock(const MappedBlock&) = delete;
        MappedBlock& operator=(const MappedBlock&) = delete;
    };

    struct ArenaState {
        void* base;
        std::vector<void*> slotAddresses;
        std::vector<bool> occupied;
        struct SlotInfo { MSDFCache* owner; uint32_t blockId; };
        std::vector<SlotInfo> slotMap;
        uint32_t nextSlotIndex;
        size_t effectiveSlotSize = 0;

        void* GetFreeSlot(MSDFCache* owner, uint32_t blockId, uint32_t& outIndex);
        void FreeSlot(uint32_t index);
        void FlushAll();

        ArenaState();
        ~ArenaState();

        size_t SlotSize() const { return effectiveSlotSize; }
    };

    bool TryLoadGlyph(uint32_t codepoint, GlyphMetrics& outMetrics);
    bool StoreGlyph(GlyphMetricsToStore&& metrics);
    size_t GetManifestSize();

    using ManifestMap = ankerl::unordered_dense::map<uint32_t, ManifestEntry>;
    using CacheMap = ankerl::unordered_dense::map<uint32_t, MappedBlock*>;

    bool LoadManifest();
    bool SaveManifest(bool isLocked = false);
    bool LoadManifestFromFile(const std::filesystem::path& path, ManifestMap& outMap);
    bool LoadManifestJournal(const std::filesystem::path& journalPath, ManifestMap& outMap, size_t& outEntriesApplied);
    bool AppendManifestJournal(const std::vector<ManifestEntry>& entries);

    void BuildBlockLockPath(uint32_t blockId, std::filesystem::path& outPath) const;
    void BuildBlockPath(uint32_t blockId, std::filesystem::path& outPath) const;

    void FlushAllBlocks();
    bool LoadMappedBlock(uint32_t blockId, MappedBlock& outBlock);
    MappedBlock* GetOrLoadMappedBlock(uint32_t blockId);
    int FindEntryIndexInBlock(const MappedBlock* block, uint32_t codepoint);

    bool FlushPendingWrites();
    bool WriteBlockFile(uint32_t blockId, std::vector<GlyphMetricsToStore*>& pendingForBlock,
        std::vector<ManifestEntry>& outNewEntries);

    static uint32_t GetBlockId(uint32_t codepoint) { return codepoint / BLOCK_SIZE; }
    static std::string GetCacheBasePath(const char* familyName, const char* styleName,
        uint32_t sdfRenderSize, uint32_t sdfSpread);
    static std::string SanitizeName(std::string_view name);

    std::filesystem::path m_cacheBasePath;
    std::filesystem::path m_cacheManifestPath;
    std::filesystem::path m_cacheManifestLockPath;
    std::filesystem::path m_cacheManifestJournalPath;

    CacheKey m_key;
    ManifestMap m_manifest;
    CacheMap m_blockCache;

    VectorPool<uint8_t> m_vecPool;
    VectorPool<GlyphEntry> m_gEntryPool;
    VectorPool<ManifestEntry> m_mEntryPool;

    bool m_manifestLoaded;
    uint32_t m_currentMappedSize;

    inline static std::deque<GlyphMetricsToStore> s_pendingWrites;

    inline static ArenaState s_arena = ArenaState();
    inline static SYSTEM_INFO s_si;
};