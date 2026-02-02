#pragma once
#include "MSDF.h"
#include "ankerl/unordered_dense.h"
#include <filesystem>
#include <deque>

class MSDFManager;
class MSDFPregen;
class MSDFFont;

class MSDFCache {
private:
    struct BlockKey {
        uint32_t fontId;
        uint32_t blockId;

        BlockKey() : fontId(0xFFFFFFFF), blockId(0xFFFFFFFF) {}
        BlockKey(uint32_t font, uint32_t block) : fontId(font), blockId(block) {}

        bool operator==(const BlockKey& other) const {
            return fontId == other.fontId && blockId == other.blockId;
        }
        uint64_t pack() const { return (static_cast<uint64_t>(fontId) << 32) | blockId; }
    };

    friend class MSDFFont;
    friend class MSDFPregen;
    friend class MSDFManager;
    friend struct std::hash<MSDFCache::BlockKey>;

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
    static constexpr size_t WRITE_BATCH_SIZE = 64;
    static constexpr size_t BLOCK_SIZE = 512;
    static constexpr size_t MAX_SAFE_ALLOCATION = 32 * 1024 * 1024;

    struct CacheKey {
        uint32_t sdfRenderSize = 0;
        uint32_t sdfSpread = 0;
        uint32_t d3dFormat = 0;
        bool operator==(const CacheKey& other) const {
            return sdfRenderSize == other.sdfRenderSize &&
                sdfSpread == other.sdfSpread &&
                d3dFormat == other.d3dFormat;
        }
    };

    struct BlockWrap {
        BlockKey key;
        std::filesystem::path path;
    };

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

    bool TryLoadGlyph(uint32_t codepoint, GlyphMetrics& outMetrics);
    bool StoreGlyph(GlyphMetricsToStore&& metrics);
    size_t GetManifestSize();

    using ManifestMap = ankerl::unordered_dense::map<uint32_t, ManifestEntry>;

    bool LoadManifest();
    bool SaveManifest(bool isLocked = false);
    bool LoadManifestFromFile(const std::filesystem::path& path, ManifestMap& outMap);
    bool LoadManifestJournal(const std::filesystem::path& journalPath, ManifestMap& outMap, size_t& outEntriesApplied);
    bool AppendManifestJournal(const std::vector<ManifestEntry>& entries);

    void BuildBlockLockPath(uint32_t blockId, std::filesystem::path& outPath) const;
    void BuildBlockPath(uint32_t blockId, std::filesystem::path& outPath) const;

    bool FlushPendingWrites();
    bool WriteBlockFile(uint32_t blockId, std::vector<GlyphMetricsToStore*>& pendingForBlock,
        std::vector<ManifestEntry>& outNewEntries);
    void CleanupOrphans();

    static uint32_t GetBlockId(uint32_t codepoint);
    static std::string GetCacheBasePath(const char* familyName, const char* styleName,
        uint32_t sdfRenderSize, uint32_t sdfSpread);
    static std::string SanitizeName(std::string_view name);

    uint32_t m_fontID;

    std::filesystem::path m_cacheBasePath;
    std::filesystem::path m_cacheManifestPath;
    std::filesystem::path m_cacheManifestLockPath;
    std::filesystem::path m_cacheManifestJournalPath;

    CacheKey m_key;
    ManifestMap m_manifest;

    bool m_manifestLoaded;

    VectorPool<uint8_t> m_vecPool;
    VectorPool<uint32_t> m_hashPool;
    VectorPool<GlyphEntry> m_gEntryPool;
    VectorPool<ManifestEntry> m_mEntryPool;

    std::deque<GlyphMetricsToStore> m_pendingWrites;
    
    ankerl::unordered_dense::map<uint32_t, BlockWrap> m_blockWrap;

    static MSDFManager s_manager;
};

namespace std {
    template<>
    struct hash<MSDFCache::BlockKey> {
        size_t operator()(const MSDFCache::BlockKey& k) const noexcept {
            uint64_t packed = k.pack();
            return ankerl::unordered_dense::detail::wyhash::hash(&packed, sizeof(packed));
        }
    };
}