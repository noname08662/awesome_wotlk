#include "MSDFCache.h"
#include "MSDFManager.h"
#include <fstream>

MSDFManager MSDFCache::s_manager = MSDFManager();

MSDFCache::MSDFCache(const FT_Byte* fontData, FT_Long dataSize, const char* familyName, const char* styleName,
    uint32_t sdfRenderSize, uint32_t sdfSpread)
    : m_key{ sdfRenderSize, sdfSpread },
    m_manifestLoaded(false),
    m_manifest(),
    m_fontID(0xFFFFFFFF)
{
    m_cacheBasePath = GetCacheBasePath(familyName, styleName, sdfRenderSize, sdfSpread);
    m_cacheManifestPath = m_cacheBasePath / "manifest.dat";
    m_cacheManifestLockPath = m_cacheBasePath / "manifest.lock";
    m_cacheManifestJournalPath = m_cacheBasePath / "manifest.jrn";

    m_fontID = s_manager.RegisterFont(HashFont(fontData, dataSize));

    std::error_code ec;
    std::filesystem::create_directories(m_cacheBasePath, ec);
}

MSDFCache::~MSDFCache() {
    FlushPendingWrites();
    CleanupOrphans();
    s_manager.FlushAll();
    m_vecPool.TrimAll();
    m_mEntryPool.TrimAll();
}

std::string MSDFCache::SanitizeName(std::string_view name) {
    if (name.empty()) return "unnamed";
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if (std::string_view("/:*?\"<>|\\").find(c) != std::string_view::npos || std::iscntrl(static_cast<unsigned char>(c))) {
            out.push_back('_');
        }
        else {
            out.push_back(c);
        }
    }
    while (!out.empty() && (out.back() == ' ' || out.back() == '.')) out.pop_back();
    return out.empty() ? "unnamed" : out;
}

std::string MSDFCache::GetCacheBasePath(const char* familyName, const char* styleName,
    uint32_t sdfRenderSize, uint32_t sdfSpread) {
    std::string fam = SanitizeName(familyName);
    std::string sty = SanitizeName(styleName);
    std::string folderName = fam + "_" + sty + "_s" + std::to_string(sdfRenderSize) + "_sp" + std::to_string(sdfSpread);
    std::filesystem::path base = std::filesystem::current_path() / CACHE_DIR / folderName;
    return base.string();
}

void MSDFCache::BuildBlockLockPath(uint32_t blockId, std::filesystem::path& outPath) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "block_%u.lock", blockId);
    outPath = m_cacheBasePath / buf;
}

void MSDFCache::BuildBlockPath(uint32_t blockId, std::filesystem::path& outPath) const {
    char buf[32];
    snprintf(buf, sizeof(buf), "block_%u.dat", blockId);
    outPath = m_cacheBasePath / buf;
}

uint32_t MSDFCache::GetBlockId(uint32_t codepoint) {
    return codepoint >> static_cast<uint32_t>(std::countr_zero(BLOCK_SIZE));
}

bool MSDFCache::TryLoadGlyph(uint32_t codepoint, GlyphMetrics& outMetrics) {
    auto mit = m_manifest.find(codepoint);
    if (mit == m_manifest.end()) return false;
    auto bit = m_blockWrap.find(mit->second.blockId);
    if (bit != m_blockWrap.end()) {
        return s_manager.LoadGlyph(bit->second, codepoint, outMetrics);
    }
    uint32_t blockId = mit->second.blockId;
    BlockKey block(m_fontID, blockId);
    std::filesystem::path blockPath;
    BuildBlockPath(blockId, blockPath);
    BlockWrap wrap = { block, blockPath };
    m_blockWrap[mit->second.blockId] = wrap;
    return s_manager.LoadGlyph(wrap, codepoint, outMetrics);
}

bool MSDFCache::StoreGlyph(GlyphMetricsToStore&& metrics) {
    if (!m_manifestLoaded) {
        if (!LoadManifest()) return false;
    }
    m_pendingWrites.push_back(std::move(metrics));
    if (m_pendingWrites.size() >= WRITE_BATCH_SIZE) {
        FlushPendingWrites();
    }
    return true;
}

bool MSDFCache::LoadManifest() {
    ScopedFileLock lock;
    if (!lock.AcquireShared(m_cacheManifestLockPath, 10000)) {
        return false;
    }

    std::error_code ec;
    bool pathExists = std::filesystem::exists(m_cacheManifestPath, ec);
    bool journalExists = std::filesystem::exists(m_cacheManifestJournalPath, ec);

    if (!pathExists && !journalExists) {
        m_manifestLoaded = true;
        return true;
    }

    if (pathExists) {
        auto fsize = std::filesystem::file_size(m_cacheManifestPath, ec);
        if (!ec && fsize > sizeof(ManifestHeader) && fsize < MAX_SAFE_ALLOCATION) {
            size_t estimatedEntries = (fsize - sizeof(ManifestHeader)) / sizeof(ManifestEntry);
            m_manifest.reserve(std::min(estimatedEntries + estimatedEntries / 10, static_cast<uint32_t>(0x110000))); // 1,114,112 - max unicode range
        }
        if (!LoadManifestFromFile(m_cacheManifestPath, m_manifest)) {
            m_manifest.clear();
            return false;
        }
    }

    size_t applied = 0;
    LoadManifestJournal(m_cacheManifestJournalPath, m_manifest, applied);

    m_manifestLoaded = true;
    return true;
}

bool MSDFCache::LoadManifestFromFile(const std::filesystem::path& path, ManifestMap& outMap) {
    std::error_code ec;
    auto fsize = std::filesystem::file_size(path, ec);
    if (ec || fsize < sizeof(ManifestHeader) || fsize > MAX_SAFE_ALLOCATION) return false;

    FileGuard file(CreateFileW(path.c_str(), GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL));
    if (!file.IsValid()) return false;

    FileGuard mapping(CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL));
    if (!mapping.IsValid()) return false;

    ViewGuard view(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
    if (!view) return false;

    const auto* hdr = reinterpret_cast<const ManifestHeader*>(view.ptr);
    if (hdr->magic == MANIFEST_MAGIC && hdr->version == CACHE_VERSION && hdr->key == m_key) {
        uint64_t expectedSize = sizeof(ManifestHeader) + static_cast<uint64_t>(hdr->entryCount) * sizeof(ManifestEntry);
        if (fsize >= expectedSize) {
            const ManifestEntry* entries = reinterpret_cast<const ManifestEntry*>(
                static_cast<const uint8_t*>(view.ptr) + sizeof(ManifestHeader));
            for (uint32_t i = 0; i < hdr->entryCount; ++i) {
                outMap.try_emplace(entries[i].codepoint, entries[i]);
            }
            return true;
        }
    }
    return false;
}

bool MSDFCache::LoadManifestJournal(const std::filesystem::path& journalPath, ManifestMap& outMap, size_t& outEntriesApplied) {
    outEntriesApplied = 0;
    std::error_code ec;
    if (!std::filesystem::exists(journalPath, ec) || ec) return true;

    auto jsize = std::filesystem::file_size(journalPath, ec);
    if (ec || jsize == 0) return !ec;

    if (jsize % sizeof(ManifestEntry) != 0) return false;
    if (jsize > MAX_SAFE_ALLOCATION) return false;

    std::ifstream in(journalPath, std::ios::binary);
    if (!in.good()) return false;

    ManifestEntry e;
    while (in.read(reinterpret_cast<char*>(&e), sizeof(ManifestEntry))) {
        outMap[e.codepoint] = e;
        ++outEntriesApplied;
    }
    return true;
}

bool MSDFCache::AppendManifestJournal(const std::vector<ManifestEntry>& entries) {
    if (entries.empty()) return true;

    std::error_code ec;
    std::filesystem::create_directories(m_cacheBasePath, ec);
    if (ec) return false;

    FileGuard file(CreateFileW(m_cacheManifestJournalPath.c_str(), FILE_APPEND_DATA,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
    if (!file.IsValid()) return false;
    if (entries.size() > UINT32_MAX / sizeof(ManifestEntry)) return false;

    DWORD totalBytes = static_cast<DWORD>(entries.size() * sizeof(ManifestEntry));
    auto buffer = m_vecPool.AcquireSized(totalBytes);
    std::memcpy(buffer.data(), entries.data(), totalBytes);

    DWORD written = 0;
    bool ok = WriteFile(file, buffer.data(), totalBytes, &written, NULL) && (written == totalBytes);
    if (ok) {
        FlushFileBuffers(file);
        file.successful = true;
    }

    m_vecPool.Release(std::move(buffer));
    return ok;
}

bool MSDFCache::SaveManifest(bool isLocked) {
    ScopedFileLock lock;
    if (!isLocked && !lock.AcquireExclusive(m_cacheManifestLockPath, 1000)) return false;

    std::filesystem::path tmpManifest = m_cacheManifestPath;
    tmpManifest.replace_extension(".tmp");

    FileGuard file(CreateFileW(tmpManifest.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL));
    if (file.handle == INVALID_HANDLE_VALUE) return false;

    ManifestHeader hdr{ MANIFEST_MAGIC, CACHE_VERSION, m_key, static_cast<uint32_t>(m_manifest.size()) };
    DWORD written = 0;

    if (!WriteFile(file.handle, &hdr, sizeof(hdr), &written, NULL)) return false;

    auto entries = m_mEntryPool.Acquire(m_manifest.size());
    entries.reserve(m_manifest.size());

    FinalAction cleanup([&]() {
        if (!entries.empty()) m_mEntryPool.Release(std::move(entries));
        });

    for (const auto& kv : m_manifest) {
        entries.push_back(ManifestEntry{ kv.first, kv.second.blockId });
    }

    if (!entries.empty()) {
        if (!WriteFile(file.handle, entries.data(), static_cast<DWORD>(entries.size() * sizeof(ManifestEntry)), &written, NULL)) {
            return false;
        }
    }
    FlushFileBuffers(file.handle);
    file.Close();

    if (MoveFileExW(tmpManifest.c_str(), m_cacheManifestPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        std::error_code ec;
        std::filesystem::remove(m_cacheManifestJournalPath, ec);
        return true;
    }
    return false;
}

size_t MSDFCache::GetManifestSize() {
    if (!m_manifestLoaded) {
        LoadManifest();
    }
    return m_manifest.size();
}

bool MSDFCache::FlushPendingWrites() {
    if (m_pendingWrites.empty()) return true;

    std::error_code ec;
    std::filesystem::create_directories(m_cacheBasePath, ec);
    if (ec) return false;

    ankerl::unordered_dense::map<uint32_t, std::vector<GlyphMetricsToStore*>> byBlock;
    for (auto& pw : m_pendingWrites) {
        byBlock[GetBlockId(pw.codepoint)].push_back(&pw);
    }
    auto newEntries = m_mEntryPool.Acquire(m_pendingWrites.size());

    size_t maxBlockSize = 0;
    for (auto& kv : byBlock) {
        maxBlockSize = std::max(maxBlockSize, kv.second.size());
    }
    auto blockEntries = m_mEntryPool.Acquire(maxBlockSize);

    for (auto& kv : byBlock) {
        uint32_t blockId = kv.first;
        std::filesystem::path lockPath;
        BuildBlockLockPath(blockId, lockPath);

        ScopedFileLock lock;
        if (!lock.AcquireExclusive(lockPath, 1000)) continue;

        blockEntries.clear();
        if (!WriteBlockFile(blockId, kv.second, blockEntries)) continue;

        newEntries.insert(newEntries.end(), blockEntries.begin(), blockEntries.end());
    }
    m_pendingWrites.clear();

    if (!newEntries.empty()) {
        ScopedFileLock lock;
        if (lock.AcquireExclusive(m_cacheManifestLockPath, 1000)) {
            if (AppendManifestJournal(newEntries)) {
                for (const ManifestEntry& me : newEntries) {
                    m_manifest[me.codepoint] = me;
                }
                SaveManifest(true);
            }
        }
    }
    m_mEntryPool.Release(std::move(blockEntries));
    m_mEntryPool.Release(std::move(newEntries));
    return true;
}

bool MSDFCache::WriteBlockFile(uint32_t blockId, std::vector<GlyphMetricsToStore*>& pending, std::vector<ManifestEntry>& outEntries) {
    std::filesystem::path blockPath;
    BuildBlockPath(blockId, blockPath);

    std::sort(pending.begin(), pending.end(), [](const GlyphMetricsToStore* a, const GlyphMetricsToStore* b) {
        return a->codepoint < b->codepoint;
        });

    auto bit = m_blockWrap.find(blockId);
    BlockWrap wrap;
    if (bit != m_blockWrap.end()) {
        wrap = bit->second;
    }
    else {
        BlockKey block(m_fontID, blockId);
        wrap = { block, blockPath };
        m_blockWrap[blockId] = wrap;
    }
    MSDFManager::MappedBlock* cachedBlock = s_manager.GetOrLoadMappedBlock(wrap);

    uint32_t oldEntriesCount = cachedBlock ? cachedBlock->entryCount : 0;
    auto mergedEntries = m_gEntryPool.Acquire(oldEntriesCount + pending.size());
    auto hashTable = m_hashPool.AcquireSized(BLOCK_SIZE, 0xFFFFFFFF);

    std::vector<uint8_t>* payloadRef = nullptr;
    FinalAction cleanup([&]() {
        if (!hashTable.empty()) m_hashPool.Release(std::move(hashTable));
        if (!mergedEntries.empty()) m_gEntryPool.Release(std::move(mergedEntries));
        if (payloadRef && !payloadRef->empty()) m_vecPool.Release(std::move(*payloadRef));
        });

    uint32_t oldIdx = 0;
    auto pendingIt = pending.begin();
    while (oldIdx < oldEntriesCount || pendingIt != pending.end()) {
        if (oldIdx < oldEntriesCount && (pendingIt == pending.end() || ((cachedBlock->entries[oldIdx].codepoint) < ((*pendingIt)->codepoint)))) {
            mergedEntries.push_back(cachedBlock->entries[oldIdx++]);
        }   
        else if (pendingIt != pending.end() && (oldIdx == oldEntriesCount || (*pendingIt)->codepoint < cachedBlock->entries[oldIdx].codepoint)) {
            auto* p = *pendingIt++;
            mergedEntries.push_back({ p->codepoint, p->width, p->height, p->bitmapTop, p->bitmapLeft, 0, p->dataSize });
        }
        else {
            auto* p = *pendingIt++;
            mergedEntries.push_back({ p->codepoint, p->width, p->height, p->bitmapTop, p->bitmapLeft, 0, p->dataSize });
            oldIdx++;
        }
    }
    if (mergedEntries.size() > BLOCK_SIZE) return false;

    uint32_t totalPayloadSize = 0;
    for (auto& ge : mergedEntries) {
        ge.dataOffset = totalPayloadSize;
        totalPayloadSize += ge.dataSize;
    }

    for (size_t i = 0; i < mergedEntries.size(); ++i) {
        hashTable[mergedEntries[i].codepoint & (BLOCK_SIZE - 1)] = static_cast<uint32_t>(i);
    }

    auto payloadBuffer = m_vecPool.AcquireSized(totalPayloadSize);
    payloadRef = &payloadBuffer;

    auto pendingCopyIt = pending.begin();
    for (auto& ge : mergedEntries) {
        const uint8_t* src = nullptr;
        while (pendingCopyIt != pending.end() && (*pendingCopyIt)->codepoint < ge.codepoint) {
            ++pendingCopyIt;
        }
        if (pendingCopyIt != pending.end() && (*pendingCopyIt)->codepoint == ge.codepoint) {
            src = (*pendingCopyIt)->ownedPixelData.data();
        }
        else if (cachedBlock) {
            uint32_t oldEntryIndex = cachedBlock->hashTable[ge.codepoint & (BLOCK_SIZE - 1)];
            if (oldEntryIndex != 0xFFFFFFFF && oldEntryIndex < cachedBlock->entryCount) {
                if (cachedBlock->entries[oldEntryIndex].codepoint == ge.codepoint) {
                    src = cachedBlock->payload + cachedBlock->entries[oldEntryIndex].dataOffset;
                }
            }
        }
        if (src && ge.dataSize > 0) {
            std::memcpy(payloadBuffer.data() + ge.dataOffset, src, ge.dataSize);
        }
    }
    s_manager.FreeBlockByKey(wrap.key);

    std::filesystem::path tmpPath = blockPath;
    tmpPath.replace_extension(".tmp");

    {
        FileGuard tmpFile(CreateFileW(tmpPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL));
        if (tmpFile.handle == INVALID_HANDLE_VALUE) return false;

        BlockFileHeader bHdr{ BLOCK_MAGIC, CACHE_VERSION, blockId, static_cast<uint32_t>(mergedEntries.size()) };
        size_t dataSize = sizeof(bHdr) + (mergedEntries.size() * sizeof(GlyphEntry)) +
            (BLOCK_SIZE * sizeof(uint32_t)) + payloadBuffer.size();


        const size_t align = MSDFManager::s_si.dwAllocationGranularity;
        size_t paddedSize = ((dataSize + align - 1) / align) * align;
        size_t paddingNeeded = paddedSize - dataSize;

        if (paddingNeeded > 0) {
            payloadBuffer.resize(payloadBuffer.size() + paddingNeeded, 0);
        }

        DWORD written;
        if (!WriteFile(tmpFile.handle, &bHdr, sizeof(bHdr), &written, NULL) || written != sizeof(bHdr)) {
            return false;
        }
        if (!WriteFile(tmpFile.handle, mergedEntries.data(),
            static_cast<DWORD>(mergedEntries.size() * sizeof(GlyphEntry)), &written, NULL)) {
            return false;
        }
        if (!WriteFile(tmpFile.handle, hashTable.data(),
            static_cast<DWORD>(BLOCK_SIZE * sizeof(uint32_t)), &written, NULL)) {
            return false;
        }
        if (!WriteFile(tmpFile.handle, payloadBuffer.data(),
            static_cast<DWORD>(payloadBuffer.size()), &written, NULL)) {
            return false;
        }
        FlushFileBuffers(tmpFile.handle);
    }

    if (!MoveFileExW(tmpPath.c_str(), blockPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        std::filesystem::path oldPath = blockPath;
        oldPath.replace_extension(".old");

        if (MoveFileExW(tmpPath.c_str(), oldPath.c_str(), MOVEFILE_REPLACE_EXISTING) || std::filesystem::exists(oldPath)) {
            auto mwit = m_blockWrap.find(blockId);
            if (mwit != m_blockWrap.end()) mwit->second.path = oldPath;
        }
        else {
            return false;
        }
    }

    for (auto* pw : pending) {
        outEntries.push_back({ pw->codepoint, blockId });
    }

    return true;
}

void MSDFCache::CleanupOrphans() {
    std::error_code ec;
    if (!std::filesystem::exists(m_cacheBasePath, ec)) return;

    for (const auto& entry : std::filesystem::directory_iterator(m_cacheBasePath, ec)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".old" || ext == ".tmp") {
                std::filesystem::remove(entry.path(), ec);
            }
        }
    }
}