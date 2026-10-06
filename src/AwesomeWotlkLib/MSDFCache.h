#pragma once

#include <ankerl/unordered_dense.h>
#include <ft2build.h>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <span>
#include <string>
#include <vector>

#include "MSDF.h"
#include "MSDFUtils.h"
#include "MSDFValidator.h"

#include FT_FREETYPE_H

class MSDFManager;

class MSDFCache {
    static constexpr uint32_t kInvalidId = 0xFFFFFFFF;

    struct BlockKey {
        uint32_t font_id;
        uint32_t block_id;

        BlockKey() : font_id(kInvalidId), block_id(kInvalidId) {}

        BlockKey(uint32_t font, uint32_t block) : font_id(font), block_id(block) {}

        bool operator==(const BlockKey& other) const { return font_id == other.font_id && block_id == other.block_id; }

        [[nodiscard]]
        uint64_t pack() const {
            return (static_cast<uint64_t>(font_id) << 32) | block_id;
        }
    };

    friend class MSDFFont;
    friend class MSDFPregen;
    friend class MSDFManager;
    friend class MSDFFaceSweep;
    friend struct std::hash<BlockKey>;

public:
    MSDFCache(const FT_Byte* font_data, FT_Long data_size, const char* family_name, const char* style_name,
        uint32_t sdf_render_size, uint32_t sdf_spread);
    ~MSDFCache();

    MSDFCache(const MSDFCache&) = delete;
    MSDFCache& operator=(const MSDFCache&) = delete;
    MSDFCache(MSDFCache&&) = delete;
    MSDFCache& operator=(MSDFCache&&) = delete;

private:
    static constexpr auto* kCacheDir = "Cache_AwesomeWotLK";
    static constexpr auto* kManifestFile = "manifest.dat";
    static constexpr auto* kManifestLockFile = "manifest.lock";
    static constexpr auto* kManifestJournalFile = "manifest.jrn";
    static constexpr uint32_t kCacheVersion = 3;
    static constexpr uint32_t kBlockMagic = 0x4D534442;
    static constexpr uint32_t kManifestMagic = 0x4D534D46;
    static constexpr uint32_t kPregenCompleteMagic = 0x47455250;  // "PREG"
    static constexpr size_t kWriteBatchSize = 64;
    static constexpr size_t kBlockSize = 512;
    static constexpr size_t kMaxSafeAllocation = 32 * 1024 * 1024;

    struct CacheKey {
        uint32_t sdf_render_size = 0;
        uint32_t sdf_spread = 0;

        bool operator==(const CacheKey& other) const {
            return sdf_render_size == other.sdf_render_size && sdf_spread == other.sdf_spread;
        }
    };

    enum class ManifestState : uint8_t { eUnloaded, eLoaded, eFailed };

    enum class FaceVerdict : uint32_t { eUnknown, eCompatible, eIncompatible };

    struct FaceRecord {
        FaceVerdict verdict = FaceVerdict::eUnknown;
        msdf_validator::FaceMetrics metrics;
        bool pregen_complete = false;
    };

    struct CacheSummary {
        size_t glyph_count = 0;
        bool pregen_complete = false;
    };

    struct BlockWrap {
        BlockKey key;
        std::filesystem::path path;
    };

#pragma pack(push, 1)

    struct alignas(64) ManifestHeader {
        uint32_t magic{};
        uint32_t version{};
        CacheKey key;
        uint32_t entry_count{};
        uint32_t face_revision{};
        FontHash font_hash{};
        FaceVerdict verdict{};
        msdf_validator::FaceMetrics metrics;
        uint32_t pregen_state{};
    };

    struct ManifestEntry {
        uint32_t codepoint;
        uint32_t block_id;
    };

    struct alignas(64) BlockFileHeader {
        uint32_t magic;
        uint32_t version;
        uint32_t block_id;
        uint32_t entry_count;
    };

    struct alignas(64) GlyphEntry {
        uint32_t codepoint;
        uint16_t width;
        uint16_t height;
        FT_Int bitmap_top;
        FT_Int bitmap_left;
        uint32_t data_offset;
        uint32_t data_size;

        bool operator<(const GlyphEntry& other) const { return codepoint < other.codepoint; }
    };

#pragma pack(pop)

    static_assert(sizeof(ManifestHeader) == 64);
    static_assert(sizeof(ManifestEntry) == 8);
    static_assert(sizeof(BlockFileHeader) == 64);
    static_assert(sizeof(GlyphEntry) == 64);

    using ManifestMap = ankerl::unordered_dense::map<uint32_t, ManifestEntry>;

    bool tryLoadGlyph(uint32_t codepoint, GlyphMetrics& out_metrics);
    size_t prefetchBlock(std::span<const uint32_t> codepoints);
    const BlockWrap& blockWrap(uint32_t block_id);
    bool storeGlyph(GlyphMetricsToStore&& metrics);
    size_t getManifestSize();
    void refresh();

    [[nodiscard]]
    FaceRecord getFaceRecord();
    void setFaceRecord(const FaceRecord& record);
    bool markPregenComplete();

    bool loadManifest();
    bool saveManifest(bool is_locked = false);

    static bool readManifest(const std::filesystem::path& manifest_path, const std::filesystem::path& journal_path,
        const CacheKey& key, FontHash font_hash, ManifestMap& out_map, FaceRecord& out_record,
        std::vector<ManifestEntry>& scratch);
    static bool writeManifest(const std::filesystem::path& manifest_path, const std::filesystem::path& journal_path,
        const CacheKey& key, FontHash font_hash, const ManifestMap& map, const FaceRecord& record,
        std::vector<ManifestEntry>& scratch);
    static bool loadManifestFromFile(const std::filesystem::path& path, const CacheKey& key, FontHash font_hash,
        ManifestMap& out_map, FaceRecord& out_record, std::vector<ManifestEntry>& scratch);
    static bool loadManifestJournal(
        const std::filesystem::path& journal_path, ManifestMap& out_map, size_t& out_entries_applied);
    [[nodiscard]]
    static CacheSummary summarizeCache(const char* family_name, const char* style_name, FontHash font_hash);
    [[nodiscard]]
    static bool readPregenComplete(const std::filesystem::path& manifest_path, const CacheKey& key, FontHash font_hash);
    [[nodiscard]]
    bool appendManifestJournal(const std::vector<ManifestEntry>& entries) const;

    [[nodiscard]]
    std::filesystem::path buildBlockLockPath(uint32_t block_id) const;
    [[nodiscard]]
    std::filesystem::path buildBlockPath(uint32_t block_id) const;

    bool flushPendingWrites();
    bool writeBlockFile(
        uint32_t block_id, std::vector<GlyphMetricsToStore*>& pending, std::vector<ManifestEntry>& out_entries);
    void cleanupOrphans() const;

    static uint32_t getBlockId(uint32_t codepoint);
    static std::string getCacheBasePath(const char* family_name, const char* style_name, uint32_t sdf_render_size,
        uint32_t sdf_spread, FontHash font_hash);

    static bool isFontBlacklisted(
        const char* family_name, const char* style_name, const uint8_t* font_data, size_t data_size);

    std::filesystem::path cache_base_path_;
    std::filesystem::path cache_manifest_path_;
    std::filesystem::path cache_manifest_lock_path_;
    std::filesystem::path cache_manifest_journal_path_;

    CacheKey key_;
    ManifestMap manifest_;
    FaceRecord face_record_;

    ManifestState manifest_state_ = ManifestState::eUnloaded;
    FontHash font_hash_ = 0;
    uint32_t font_id_ = kInvalidId;

    VectorPool<uint8_t> vec_pool_;
    VectorPool<uint32_t> hash_pool_;
    VectorPool<GlyphEntry> glyph_entry_pool_;
    VectorPool<ManifestEntry> manifest_entry_pool_;

    std::deque<GlyphMetricsToStore> pending_writes_;

    ankerl::unordered_dense::map<uint32_t, BlockWrap> block_wrap_;
};

template <>
struct std::hash<MSDFCache::BlockKey> {
    size_t operator()(const MSDFCache::BlockKey& k) const noexcept {
        uint64_t packed = k.pack();
        return ankerl::unordered_dense::detail::hash_int(packed);
    }
};
