#include "MSDFCache.h"

#include <hookkit/accessor.h>

#include <algorithm>
#include <bit>
#include <cctype>
#include <cstring>
#include <format>
#include <fstream>
#include <ranges>
#include <span>
#include <string_view>

#include "MSDFManager.h"

#include "include/Client/Client.h"

namespace {
constexpr auto* kBlacklistDir = "Fonts_AwesomeWotLK";

using HashSet = ankerl::unordered_dense::set<FontHash>;

std::string sanitizeName(std::string_view name) {
    if (name.empty()) { return "unnamed"; }
    std::string out;
    out.reserve(name.size());
    for (const char c : name) {
        if (std::string_view("/:*?\"<>|\\").find(c) != std::string_view::npos ||
            std::iscntrl(static_cast<unsigned char>(c)) != 0) {
            out.push_back('_');
        } else {
            out.push_back(c);
        }
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '.')) {
        out.pop_back();
    }
    return out.empty() ? "unnamed" : out;
}

void lowerInPlace(std::string& s) {
    std::ranges::transform(s, s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
}

uint64_t hashNormalizedString(std::string_view str) {
    std::string normalized = sanitizeName(str);
    lowerInPlace(normalized);
    return hashFont(reinterpret_cast<const FT_Byte*>(normalized.data()), static_cast<FT_Long>(normalized.size()));
}

HashSet loadBlacklist() {
    HashSet hashes;

    const std::filesystem::path blacklist_dir = std::filesystem::current_path() / kBlacklistDir;
    std::error_code ec;

    if (!std::filesystem::exists(blacklist_dir, ec)) {
        std::filesystem::create_directories(blacklist_dir, ec);
        return hashes;
    }

    for (const auto& entry : std::filesystem::directory_iterator(blacklist_dir, ec)) {
        if (!entry.is_regular_file()) { continue; }

        std::string ext = entry.path().extension().string();
        lowerInPlace(ext);

        if (ext == ".ttf" || ext == ".otf") {
            std::ifstream file(entry.path(), std::ios::binary | std::ios::ate);
            if (file) {
                const std::streamsize size = file.tellg();
                if (size > 0) {
                    std::vector<uint8_t> buffer(size);
                    file.seekg(0, std::ios::beg);
                    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                        hashes.insert(hashFont(buffer.data(), static_cast<FT_Long>(buffer.size())));
                    }
                }
            }
        }

        const std::string name_without_ext = entry.path().stem().string();
        if (!name_without_ext.empty()) { hashes.insert(hashNormalizedString(name_without_ext)); }
    }
    return hashes;
}

constexpr utils::Accessor<HashSet, struct BlacklistHashesTag, loadBlacklist> kBlacklistHashes;
}  // namespace

MSDFCache::MSDFCache(const FT_Byte* font_data, FT_Long data_size, const char* family_name, const char* style_name,
    uint32_t sdf_render_size, uint32_t sdf_spread)
    : key_{.sdf_render_size = sdf_render_size, .sdf_spread = sdf_spread},
      font_hash_(hashFont(font_data, data_size)),
      font_id_(MSDFManager::registerFont(font_hash_)) {
    cache_base_path_ = getCacheBasePath(family_name, style_name, sdf_render_size, sdf_spread, font_hash_);
    cache_manifest_path_ = cache_base_path_ / kManifestFile;
    cache_manifest_lock_path_ = cache_base_path_ / kManifestLockFile;
    cache_manifest_journal_path_ = cache_base_path_ / kManifestJournalFile;

    std::error_code ec;
    std::filesystem::create_directories(cache_base_path_, ec);
}

MSDFCache::~MSDFCache() {
    try {
        flushPendingWrites();
        cleanupOrphans();
    } catch (...) { pending_writes_.clear(); }
    MSDFManager::flushAll();
    vec_pool_.trimAll();
    manifest_entry_pool_.trimAll();
}

bool MSDFCache::tryLoadGlyph(uint32_t codepoint, GlyphMetrics& out_metrics) {
    if (manifest_state_ == ManifestState::eUnloaded) { static_cast<void>(loadManifest()); }
    const auto mit = manifest_.find(codepoint);
    if (mit == manifest_.end()) { return false; }
    return MSDFManager::loadGlyph(blockWrap(mit->second.block_id), codepoint, out_metrics);
}

size_t MSDFCache::prefetchBlock(std::span<const uint32_t> codepoints) {
    if (codepoints.empty()) { return 0; }
    const uint32_t block_id = getBlockId(codepoints.front());
    const auto run_end =
        std::ranges::find_if(codepoints, [block_id](uint32_t codepoint) { return getBlockId(codepoint) != block_id; });
    const std::span<const uint32_t> run = codepoints.first(static_cast<size_t>(run_end - codepoints.begin()));

    if (manifest_state_ == ManifestState::eUnloaded) { static_cast<void>(loadManifest()); }
    if (std::ranges::any_of(run, [this](uint32_t codepoint) { return manifest_.contains(codepoint); })) {
        MSDFManager::prefetchGlyphs(blockWrap(block_id), run);
    }
    return run.size();
}

const MSDFCache::BlockWrap& MSDFCache::blockWrap(uint32_t block_id) {
    auto [it, inserted] = block_wrap_.try_emplace(block_id);
    if (inserted) { it->second = {.key = BlockKey(font_id_, block_id), .path = buildBlockPath(block_id)}; }
    return it->second;
}

bool MSDFCache::storeGlyph(GlyphMetricsToStore&& metrics) {
    if (manifest_state_ != ManifestState::eLoaded && !loadManifest()) { return false; }
    pending_writes_.push_back(std::move(metrics));
    if (pending_writes_.size() >= kWriteBatchSize) { flushPendingWrites(); }
    return true;
}

size_t MSDFCache::getManifestSize() {
    if (manifest_state_ != ManifestState::eLoaded) { loadManifest(); }
    return manifest_.size();
}

void MSDFCache::refresh() {
    pending_writes_.clear();
    manifest_.clear();
    block_wrap_.clear();
    face_record_ = {};
    manifest_state_ = ManifestState::eUnloaded;
}

MSDFCache::FaceRecord MSDFCache::getFaceRecord() {
    if (manifest_state_ == ManifestState::eUnloaded) { static_cast<void>(loadManifest()); }
    return face_record_;
}

void MSDFCache::setFaceRecord(const FaceRecord& record) {
    if (manifest_state_ == ManifestState::eUnloaded) { static_cast<void>(loadManifest()); }
    face_record_.verdict = record.verdict;
    face_record_.metrics = record.metrics;
    if (manifest_state_ == ManifestState::eLoaded) { static_cast<void>(saveManifest()); }
}

bool MSDFCache::markPregenComplete() {
    if (manifest_state_ != ManifestState::eLoaded && !loadManifest()) { return false; }
    face_record_.pregen_complete = true;
    return saveManifest();
}

bool MSDFCache::loadManifest() {
    const auto [verdict, metrics, pregen_complete] = face_record_;
    auto scratch = manifest_entry_pool_.acquire(0);
    const bool read = readManifest(
        cache_manifest_path_, cache_manifest_journal_path_, key_, font_hash_, manifest_, face_record_, scratch);
    manifest_entry_pool_.release(std::move(scratch));
    if (face_record_.verdict == FaceVerdict::eUnknown) {
        face_record_.verdict = verdict;
        face_record_.metrics = metrics;
    }
    face_record_.pregen_complete = face_record_.pregen_complete || pregen_complete;
    if (!read) {
        manifest_state_ = ManifestState::eFailed;
        return false;
    }
    manifest_state_ = ManifestState::eLoaded;
    return true;
}

bool MSDFCache::saveManifest(bool is_locked) {
    ScopedFileLock lock;
    if (!is_locked && !lock.acquireExclusive(cache_manifest_lock_path_, 1000)) { return false; }
    // another instance (pregen, a second client) may have marked it since this one loaded
    if (!face_record_.pregen_complete) {
        face_record_.pregen_complete = readPregenComplete(cache_manifest_path_, key_, font_hash_);
    }
    auto scratch = manifest_entry_pool_.acquire(manifest_.size());
    const bool saved = writeManifest(
        cache_manifest_path_, cache_manifest_journal_path_, key_, font_hash_, manifest_, face_record_, scratch);
    manifest_entry_pool_.release(std::move(scratch));
    return saved;
}

bool MSDFCache::readManifest(const std::filesystem::path& manifest_path, const std::filesystem::path& journal_path,
    const CacheKey& key, FontHash font_hash, ManifestMap& out_map, FaceRecord& out_record,
    std::vector<ManifestEntry>& scratch) {
    out_map.clear();
    out_record = {};

    std::error_code ec;
    const auto fsize = std::filesystem::file_size(manifest_path, ec);
    const bool path_exists = !ec;
    if (path_exists && fsize > sizeof(ManifestHeader) && fsize < kMaxSafeAllocation) {
        const size_t estimated_entries = (fsize - sizeof(ManifestHeader)) / sizeof(ManifestEntry);
        constexpr size_t kMaxUnicodeRange = 0x110000;  // 1,114,112
        out_map.reserve(std::min(estimated_entries + estimated_entries / 10, kMaxUnicodeRange));
    }

    size_t applied = 0;
    loadManifestJournal(journal_path, out_map, applied);

    if (path_exists && !loadManifestFromFile(manifest_path, key, font_hash, out_map, out_record, scratch)) {
        out_map.clear();
        return false;
    }
    return true;
}

bool MSDFCache::writeManifest(const std::filesystem::path& manifest_path, const std::filesystem::path& journal_path,
    const CacheKey& key, FontHash font_hash, const ManifestMap& map, const FaceRecord& record,
    std::vector<ManifestEntry>& scratch) {
    std::filesystem::path tmp_manifest = manifest_path;
    tmp_manifest.replace_extension(".tmp");

    FileGuard file(
        CreateFileW(tmp_manifest.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (file.handle == INVALID_HANDLE_VALUE) { return false; }

    const ManifestHeader hdr{
        .magic = kManifestMagic,
        .version = kCacheVersion,
        .key = key,
        .entry_count = map.size(),
        .face_revision = msdf_validator::kRevision,
        .font_hash = font_hash,
        .verdict = record.verdict,
        .metrics = record.metrics,
        .pregen_state = record.pregen_complete ? kPregenCompleteMagic : 0
    };
    DWORD written = 0;

    if (WriteFile(file.handle, &hdr, sizeof(hdr), &written, nullptr) == 0) { return false; }

    scratch.clear();
    scratch.reserve(map.size());
    for (const auto& [codepoint, entry] : map) {
        scratch.push_back({.codepoint = codepoint, .block_id = entry.block_id});
    }

    if (!scratch.empty() &&
        WriteFile(file.handle, scratch.data(), scratch.size() * sizeof(ManifestEntry), &written, nullptr) == 0) {
        return false;
    }
    FlushFileBuffers(file.handle);
    file.close();

    if (MoveFileExW(tmp_manifest.c_str(), manifest_path.c_str(), MOVEFILE_REPLACE_EXISTING) != 0) {
        std::error_code ec;
        std::filesystem::remove(journal_path, ec);
        return true;
    }
    return false;
}

bool MSDFCache::loadManifestFromFile(const std::filesystem::path& path, const CacheKey& key, FontHash font_hash,
    ManifestMap& out_map, FaceRecord& out_record, std::vector<ManifestEntry>& scratch) {
    constexpr int kOpenAttempts = 8;
    auto* handle = INVALID_HANDLE_VALUE;
    for (int attempt = 0; attempt < kOpenAttempts; ++attempt) {
        handle = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
        if (handle != INVALID_HANDLE_VALUE || GetLastError() != ERROR_SHARING_VIOLATION) { break; }
        Sleep(attempt == 0 ? 0 : 1);
    }
    FileGuard file(handle);
    if (!file.isValid()) { return false; }

    LARGE_INTEGER size{};
    if (GetFileSizeEx(file.handle, &size) == 0) { return false; }
    if (std::cmp_less(size.QuadPart, static_cast<LONGLONG>(sizeof(ManifestHeader))) ||
        size.QuadPart > static_cast<int>(kMaxSafeAllocation)) {
        return true;
    }

    ManifestHeader hdr;
    DWORD read = 0;
    if (ReadFile(file.handle, &hdr, sizeof(hdr), &read, nullptr) == 0) { return false; }
    if (read != sizeof(hdr) || hdr.magic != kManifestMagic || hdr.version != kCacheVersion || hdr.key != key ||
        hdr.font_hash != font_hash) {
        return true;
    }
    if (hdr.face_revision == msdf_validator::kRevision) {
        out_record.verdict = hdr.verdict;
        out_record.metrics = hdr.metrics;
    }
    out_record.pregen_complete = hdr.pregen_state == kPregenCompleteMagic;

    const uint64_t entries_size = static_cast<uint64_t>(hdr.entry_count) * sizeof(ManifestEntry);
    if (hdr.entry_count == 0 || static_cast<uint64_t>(size.QuadPart) - sizeof(ManifestHeader) < entries_size) {
        return true;
    }
    scratch.resize(hdr.entry_count);
    if (ReadFile(file.handle, scratch.data(), static_cast<DWORD>(entries_size), &read, nullptr) == 0) { return false; }
    if (read != entries_size) { return true; }
    for (const ManifestEntry& e : scratch) {
        out_map.try_emplace(e.codepoint, e);
    }
    return true;
}

bool MSDFCache::loadManifestJournal(
    const std::filesystem::path& journal_path, ManifestMap& out_map, size_t& out_entries_applied) {
    out_entries_applied = 0;
    std::error_code ec;
    if (!std::filesystem::exists(journal_path, ec) || ec) { return true; }

    const auto jsize = std::filesystem::file_size(journal_path, ec);
    if (ec || jsize == 0) { return !ec; }
    if (jsize > kMaxSafeAllocation) { return false; }

    std::ifstream in(journal_path, std::ios::binary);
    if (!in.good()) { return false; }

    ManifestEntry e{};
    while (in.read(reinterpret_cast<char*>(&e), sizeof(ManifestEntry))) {
        if (e.block_id != getBlockId(e.codepoint)) { continue; }
        out_map[e.codepoint] = e;
        ++out_entries_applied;
    }
    return true;
}

MSDFCache::CacheSummary MSDFCache::summarizeCache(const char* family_name, const char* style_name, FontHash font_hash) {
    constexpr CacheKey kKey{.sdf_render_size = msdf::kSdfRenderSize, .sdf_spread = msdf::kSdfSpread};
    const std::filesystem::path base =
        getCacheBasePath(family_name, style_name, kKey.sdf_render_size, kKey.sdf_spread, font_hash);
    ManifestMap map;
    FaceRecord record;
    std::vector<ManifestEntry> scratch;
    if (!readManifest(base / kManifestFile, base / kManifestJournalFile, kKey, font_hash, map, record, scratch)) {
        return {};
    }
    return {.glyph_count = map.size(), .pregen_complete = record.pregen_complete};
}

bool MSDFCache::readPregenComplete(
    const std::filesystem::path& manifest_path, const CacheKey& key, FontHash font_hash) {
    std::ifstream in(manifest_path, std::ios::binary);
    ManifestHeader hdr;
    if (!in.read(reinterpret_cast<char*>(&hdr), sizeof(hdr))) { return false; }
    return hdr.magic == kManifestMagic && hdr.version == kCacheVersion && hdr.key == key &&
        hdr.font_hash == font_hash && hdr.pregen_state == kPregenCompleteMagic;
}

bool MSDFCache::appendManifestJournal(const std::vector<ManifestEntry>& entries) const {
    if (entries.empty()) { return true; }

    std::error_code ec;
    std::filesystem::create_directories(cache_base_path_, ec);
    if (ec) { return false; }

    FileGuard file(CreateFileW(cache_manifest_journal_path_.c_str(), FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!file.isValid()) { return false; }
    if (entries.size() > UINT32_MAX / sizeof(ManifestEntry)) { return false; }

    const DWORD total_bytes = static_cast<DWORD>(entries.size()) * static_cast<DWORD>(sizeof(ManifestEntry));
    DWORD written = 0;
    const bool ok =
        (WriteFile(file.handle, entries.data(), total_bytes, &written, nullptr) != 0) && (written == total_bytes);
    if (ok) {
        FlushFileBuffers(file.handle);
        file.successful = true;
    }
    return ok;
}

std::filesystem::path MSDFCache::buildBlockLockPath(uint32_t block_id) const {
    return cache_base_path_ / ("block_" + std::to_string(block_id) + ".lock");
}

std::filesystem::path MSDFCache::buildBlockPath(uint32_t block_id) const {
    return cache_base_path_ / ("block_" + std::to_string(block_id) + ".dat");
}

bool MSDFCache::flushPendingWrites() {
    if (pending_writes_.empty()) { return true; }

    std::error_code ec;
    std::filesystem::create_directories(cache_base_path_, ec);
    if (ec) { return false; }

    ankerl::unordered_dense::map<uint32_t, std::vector<GlyphMetricsToStore*>> by_block;
    for (auto& pw : pending_writes_) {
        by_block[getBlockId(pw.codepoint)].push_back(&pw);
    }
    auto new_entries = manifest_entry_pool_.acquire(pending_writes_.size());

    const size_t max_block_size = by_block.empty()
        ? 0
        : std::ranges::max(by_block | std::views::values | std::views::transform([](auto& v) { return v.size(); }));
    auto block_entries = manifest_entry_pool_.acquire(max_block_size);

    for (auto& [block_id, pending] : by_block) {
        ScopedFileLock lock;
        if (!lock.acquireExclusive(buildBlockLockPath(block_id), 1000)) { continue; }

        block_entries.clear();
        if (!writeBlockFile(block_id, pending, block_entries)) { continue; }

        new_entries.insert(new_entries.end(), block_entries.begin(), block_entries.end());
    }
    pending_writes_.clear();

    if (!new_entries.empty()) {
        ScopedFileLock lock;
        if (lock.acquireExclusive(cache_manifest_lock_path_, 1000) && appendManifestJournal(new_entries)) {
            for (const ManifestEntry& me : new_entries) {
                manifest_[me.codepoint] = me;
            }
            saveManifest(true);
        }
    }
    manifest_entry_pool_.release(std::move(block_entries));
    manifest_entry_pool_.release(std::move(new_entries));
    return true;
}

bool MSDFCache::writeBlockFile(
    uint32_t block_id, std::vector<GlyphMetricsToStore*>& pending, std::vector<ManifestEntry>& out_entries) {
    const std::filesystem::path block_path = buildBlockPath(block_id);

    std::ranges::sort(pending, {}, &GlyphMetricsToStore::codepoint);

    BlockWrap wrap;
    if (const auto bit = block_wrap_.find(block_id); bit != block_wrap_.end()) {
        wrap = bit->second;
    } else {
        wrap = {.key = BlockKey(font_id_, block_id), .path = block_path};
        block_wrap_[block_id] = wrap;
    }
    std::span<const GlyphEntry> old_entries;
    std::span<const uint32_t> old_hash_table;
    std::span<const uint8_t> old_payload;
    if (const MSDFManager::MappedBlock* cached_block = MSDFManager::getOrLoadMappedBlock(wrap)) {
        old_entries = cached_block->entries;
        old_hash_table = cached_block->hash_table;
        old_payload = cached_block->payload;
    }

    auto merged_entries = glyph_entry_pool_.acquire(old_entries.size() + pending.size());
    auto hash_table = hash_pool_.acquireSized(kBlockSize, kInvalidId);

    std::vector<uint8_t>* payload_ref = nullptr;
    FinalAction cleanup([&]() {
        if (!hash_table.empty()) { hash_pool_.release(std::move(hash_table)); }
        if (!merged_entries.empty()) { glyph_entry_pool_.release(std::move(merged_entries)); }
        if (payload_ref != nullptr && !payload_ref->empty()) { vec_pool_.release(std::move(*payload_ref)); }
    });

    const auto make_entry = [](const GlyphMetricsToStore* p) {
        return GlyphEntry{
            .codepoint = p->codepoint,
            .width = p->width,
            .height = p->height,
            .bitmap_top = p->bitmap_top,
            .bitmap_left = p->bitmap_left,
            .data_offset = 0,
            .data_size = p->data_size
        };
    };

    size_t old_idx = 0;
    auto pending_it = pending.begin();
    while (old_idx < old_entries.size() || pending_it != pending.end()) {
        if (old_idx < old_entries.size() &&
            (pending_it == pending.end() || old_entries[old_idx].codepoint < (*pending_it)->codepoint)) {
            merged_entries.push_back(old_entries[old_idx++]);
        } else if (pending_it != pending.end() &&
            (old_idx == old_entries.size() || (*pending_it)->codepoint < old_entries[old_idx].codepoint)) {
            merged_entries.push_back(make_entry(*pending_it++));
        } else {
            merged_entries.push_back(make_entry(*pending_it++));
            old_idx++;
        }
    }
    if (merged_entries.size() > kBlockSize) { return false; }

    uint32_t total_payload_size = 0;
    for (auto& ge : merged_entries) {
        ge.data_offset = total_payload_size;
        total_payload_size += ge.data_size;
    }

    for (size_t i = 0; i < merged_entries.size(); ++i) {
        hash_table[merged_entries[i].codepoint & (kBlockSize - 1)] = i;
    }

    auto payload_buffer = vec_pool_.acquireSized(total_payload_size);
    payload_ref = &payload_buffer;

    const std::span payload_out(payload_buffer);
    auto pending_copy_it = pending.begin();
    for (const auto& ge : merged_entries) {
        if (ge.data_size == 0) { continue; }
        const uint8_t* src = nullptr;
        while (pending_copy_it != pending.end() && (*pending_copy_it)->codepoint < ge.codepoint) {
            ++pending_copy_it;
        }
        if (pending_copy_it != pending.end() && (*pending_copy_it)->codepoint == ge.codepoint) {
            src = (*pending_copy_it)->owned_pixel_data.data();
        } else if (!old_hash_table.empty()) {
            const uint32_t old_entry_index = old_hash_table[ge.codepoint & (kBlockSize - 1)];
            if (old_entry_index < old_entries.size() && old_entries[old_entry_index].codepoint == ge.codepoint) {
                src = old_payload.subspan(old_entries[old_entry_index].data_offset).data();
            }
        }
        if (src != nullptr) { std::memcpy(payload_out.subspan(ge.data_offset).data(), src, ge.data_size); }
    }
    MSDFManager::freeBlockByKey(wrap.key);

    std::filesystem::path tmp_path = block_path;
    tmp_path.replace_extension(".tmp");

    {
        FileGuard tmp_file(CreateFileW(tmp_path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, nullptr));
        if (tmp_file.handle == INVALID_HANDLE_VALUE) { return false; }

        const BlockFileHeader block_hdr{
            .magic = kBlockMagic, .version = kCacheVersion, .block_id = block_id, .entry_count = merged_entries.size()};
        const size_t data_size = sizeof(block_hdr) + (merged_entries.size() * sizeof(GlyphEntry)) +
            (kBlockSize * sizeof(uint32_t)) + payload_buffer.size();

        const size_t align = MSDFManager::kSystemInfo->dwAllocationGranularity;
        const size_t padded_size = ((data_size + align - 1) / align) * align;
        const size_t padding_needed = padded_size - data_size;

        if (padding_needed > 0) { payload_buffer.resize(payload_buffer.size() + padding_needed, 0); }

        DWORD written;
        if ((WriteFile(tmp_file.handle, &block_hdr, sizeof(block_hdr), &written, nullptr) == 0) ||
            written != sizeof(block_hdr)) {
            return false;
        }
        if (WriteFile(tmp_file.handle, merged_entries.data(), merged_entries.size() * sizeof(GlyphEntry), &written,
                nullptr) == 0) {
            return false;
        }
        if (WriteFile(tmp_file.handle, hash_table.data(), kBlockSize * sizeof(uint32_t), &written, nullptr) == 0) {
            return false;
        }
        if (WriteFile(tmp_file.handle, payload_buffer.data(), payload_buffer.size(), &written, nullptr) == 0) {
            return false;
        }
        FlushFileBuffers(tmp_file.handle);
    }

    if (MoveFileExW(tmp_path.c_str(), block_path.c_str(), MOVEFILE_REPLACE_EXISTING) == 0) {
        std::filesystem::path old_path = block_path;
        old_path.replace_extension(".old");

        std::error_code ec;
        if ((MoveFileExW(tmp_path.c_str(), old_path.c_str(), MOVEFILE_REPLACE_EXISTING) == 0) &&
            !std::filesystem::exists(old_path, ec)) {
            return false;
        }
        if (const auto mwit = block_wrap_.find(block_id); mwit != block_wrap_.end()) { mwit->second.path = old_path; }
    }

    for (const auto* pw : pending) {
        out_entries.push_back({.codepoint = pw->codepoint, .block_id = block_id});
    }

    return true;
}

void MSDFCache::cleanupOrphans() const {
    std::error_code ec;
    if (!std::filesystem::exists(cache_base_path_, ec)) { return; }

    for (const auto& entry : std::filesystem::directory_iterator(cache_base_path_, ec)) {
        if (entry.is_regular_file(ec)) {
            const std::filesystem::path ext = entry.path().extension();
            if (ext == ".old" || ext == ".tmp") { std::filesystem::remove(entry.path(), ec); }
        }
    }
}

uint32_t MSDFCache::getBlockId(uint32_t codepoint) {
    return codepoint >> static_cast<uint32_t>(std::countr_zero(kBlockSize));
}

std::string MSDFCache::getCacheBasePath(const char* family_name, const char* style_name, uint32_t sdf_render_size,
    uint32_t sdf_spread, FontHash font_hash) {
    const std::string folder_name = std::format("{}_{}_s{}_sp{}_{:016x}", sanitizeName(family_name),
        sanitizeName(style_name), sdf_render_size, sdf_spread, font_hash);
    return (std::filesystem::current_path() / kCacheDir / "Fonts" / client::getGameLocale() / folder_name).string();
}

bool MSDFCache::isFontBlacklisted(
    const char* family_name, const char* style_name, const uint8_t* font_data, size_t data_size) {
    const HashSet& hashes = *kBlacklistHashes;
    if (hashes.empty()) { return false; }
    if (font_data != nullptr && data_size > 0 &&
        hashes.contains(hashFont(font_data, static_cast<FT_Long>(data_size)))) {
        return true;
    }
    if (family_name != nullptr && hashes.contains(hashNormalizedString(family_name))) { return true; }
    if (family_name != nullptr && style_name != nullptr) {
        const std::string combined = std::string(family_name) + "_" + style_name;
        if (hashes.contains(hashNormalizedString(combined))) { return true; }
    }
    return false;
}
