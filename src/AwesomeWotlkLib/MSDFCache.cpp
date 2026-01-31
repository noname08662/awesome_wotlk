#include "MSDFCache.h"
#include <fstream>

#pragma comment(lib, "onecore.lib")

MSDFCache::MSDFCache(const FT_Byte* fontData, FT_Long dataSize, const char* familyName, const char* styleName,
    uint32_t sdfRenderSize, uint32_t sdfSpread)
    : m_key{ sdfRenderSize, sdfSpread },
    m_manifestLoaded(false),
    m_manifest(), m_blockCache()
{
    m_cacheBasePath = GetCacheBasePath(familyName, styleName, sdfRenderSize, sdfSpread);
    m_cacheManifestPath = m_cacheBasePath / "manifest.dat";
    m_cacheManifestLockPath = m_cacheBasePath / "manifest.lock";
    m_cacheManifestJournalPath = m_cacheBasePath / "manifest.jrn";

    std::error_code ec;
    std::filesystem::create_directories(m_cacheBasePath, ec);
}

MSDFCache::~MSDFCache() {
    FlushPendingWrites();
    for (auto& pair : m_blockCache) {
        pair.second->Close();
        delete pair.second;
    }
    m_blockCache.clear();
    m_vecPool.TrimAll();
    m_mEntryPool.TrimAll();
    m_gEntryPool.TrimAll();
}

std::string MSDFCache::SanitizeName(std::string_view name) {
    if (name.empty()) return "unnamed";
    std::string out;
    out.reserve(name.size());
    for (char c : name) {
        if (std::string_view("/:*?\"<>|\\").find(c) != std::string_view::npos) {
            out.push_back('_');
        }
        else {
            out.push_back(c);
        }
    }
    return out;
}

std::string MSDFCache::GetCacheBasePath(const char* familyName, const char* styleName,
    uint32_t sdfRenderSize, uint32_t sdfSpread) {
    std::string fam = SanitizeName(familyName);
    std::string sty = SanitizeName(styleName);
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s_%s_s%u_sp%u",
        fam.c_str(), sty.c_str(), sdfRenderSize, sdfSpread);
    std::filesystem::path base = std::filesystem::current_path() / CACHE_DIR / buffer;
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

void MSDFCache::MappedBlock::Close() {
    view.Close();
    mapping.Close();
    file.Close();
    header = nullptr;
    entries = nullptr;
    payload = nullptr;
    entryCount = 0;
    if (isStale && !stalePath.empty()) {
        std::error_code ec;
        std::filesystem::remove(stalePath, ec);
    }
}

// 32bit sucks
MSDFCache::ArenaState::ArenaState() {
    const uint32_t maxGlyphDim = MSDF::SDF_RENDER_SIZE + 2 * MSDF::SDF_SPREAD;
    const uint32_t maxPixelsPerGlyph = maxGlyphDim * maxGlyphDim;
    const uint32_t maxBytesPerGlyph = maxPixelsPerGlyph * 4;

    const size_t maxPayload = BLOCK_SIZE * maxBytesPerGlyph;
    const size_t maxEntries = BLOCK_SIZE * sizeof(GlyphEntry);
    const size_t maxBlockSize = sizeof(BlockFileHeader) + maxEntries + maxPayload;

    GetSystemInfo(&s_si);
    const size_t gran = s_si.dwAllocationGranularity;
    effectiveSlotSize = ((maxBlockSize + gran - 1) / gran) * gran;

    slotAddresses.resize(MAX_ARENA_SLOTS);
    occupied.assign(MAX_ARENA_SLOTS, false);
    slotMap.assign(MAX_ARENA_SLOTS, { nullptr, 0 });
    nextSlotIndex = 0;

    const size_t totalSize = effectiveSlotSize * MAX_ARENA_SLOTS;
    base = VirtualAlloc2(GetCurrentProcess(), NULL, totalSize,
        MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
        PAGE_NOACCESS, NULL, 0);
    if (!base) return;

    for (size_t i = 0; i < MAX_ARENA_SLOTS; ++i) {
        void* slotAddr = static_cast<char*>(base) + (i * effectiveSlotSize);
        slotAddresses[i] = slotAddr;
        VirtualFreeEx(GetCurrentProcess(), slotAddr, effectiveSlotSize,
            MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    }
}


MSDFCache::ArenaState::~ArenaState() {
    if (base) {
        VirtualFreeEx(GetCurrentProcess(), base, 0, MEM_RELEASE);
        base = nullptr;
    }
}

void* MSDFCache::ArenaState::GetFreeSlot(MSDFCache* owner, uint32_t blockId, uint32_t& outIndex) {
    if (nextSlotIndex >= MAX_ARENA_SLOTS) return nullptr;
    uint32_t i = nextSlotIndex++;
    occupied[i] = true;
    slotMap[i] = { owner, blockId };
    outIndex = i;
    return slotAddresses[i];
}

void MSDFCache::ArenaState::FreeSlot(uint32_t index) {
    if (index >= slotAddresses.size()) return;
    void* slotAddr = slotAddresses[index];
    uintptr_t currentAddr = reinterpret_cast<uintptr_t>(slotAddr);
    uintptr_t endAddr = currentAddr + effectiveSlotSize;
    while (currentAddr < endAddr) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(reinterpret_cast<void*>(currentAddr), &mbi, sizeof(mbi)) == 0) break;
        if (reinterpret_cast<uintptr_t>(mbi.BaseAddress) >= endAddr) break;

        size_t regionSize = mbi.RegionSize;
        if (mbi.State != MEM_FREE) {
            if (mbi.Type == MEM_MAPPED) {
                UnmapViewOfFile2(GetCurrentProcess(), mbi.BaseAddress, 0);
            }
            else {
                VirtualFree(mbi.BaseAddress, 0, MEM_RELEASE);
            }
        }
        currentAddr += regionSize;
    }
    VirtualFreeEx(GetCurrentProcess(), slotAddr, 0, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS);
    VirtualAlloc2(GetCurrentProcess(), slotAddr, effectiveSlotSize,
        MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
        PAGE_NOACCESS, NULL, 0);
    occupied[index] = false;
    slotMap[index] = { nullptr, 0 };
}

void MSDFCache::ArenaState::FlushAll() {
    for (uint32_t i = 0; i < MAX_ARENA_SLOTS; ++i) {
        if (occupied[i]) FreeSlot(i);
    }
    nextSlotIndex = 0;
}

void MSDFCache::FreeBlock(uint32_t blockId, uint32_t index) {
    auto cit = m_blockCache.find(blockId);
    if (cit != m_blockCache.end()) {
        cit->second->Close();
        delete cit->second;
        m_blockCache.erase(cit);
    }
    s_arena.FreeSlot(index);
}

void MSDFCache::FlushAllBlocks() {
    for (auto& [id, blockPtr] : m_blockCache) {
        if (blockPtr) {
            blockPtr->Close();
            delete blockPtr;
        }
    }
    m_blockCache.clear();
    s_arena.FlushAll();
}

bool MSDFCache::LoadMappedBlock(uint32_t blockId, MappedBlock& outBlock) {
    std::filesystem::path blockPath;
    BuildBlockPath(blockId, blockPath);

    outBlock.file.handle = CreateFileW(blockPath.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (outBlock.file.handle == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER fileSizeLI;
    if (!GetFileSizeEx(outBlock.file.handle, &fileSizeLI)) return false;

    outBlock.fileSize = static_cast<uint64_t>(fileSizeLI.QuadPart);

    const size_t allocGran = s_si.dwAllocationGranularity;
    size_t splitSize = ((outBlock.fileSize + allocGran - 1) / allocGran) * allocGran;
    if (splitSize < allocGran) splitSize = allocGran;
    if (splitSize > s_arena.SlotSize()) return false;

    uint32_t index = 0;
    void* targetAddr = s_arena.GetFreeSlot(this, blockId, index);
    if (!targetAddr) return false;

    if (!VirtualFreeEx(GetCurrentProcess(), targetAddr, splitSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
        FreeBlock(blockId, index);
        return false;
    }

    DWORD sizeHigh = static_cast<DWORD>(splitSize >> 32);
    DWORD sizeLow = static_cast<DWORD>(splitSize & 0xFFFFFFFF);
    outBlock.mapping.handle = CreateFileMappingW(outBlock.file.handle, NULL, PAGE_READWRITE, sizeHigh, sizeLow, NULL);
    if (!outBlock.mapping.handle) {
        FreeBlock(blockId, index);
        return false;
    }

    outBlock.view.ptr = MapViewOfFile3(outBlock.mapping.handle, NULL, targetAddr, 0, splitSize,
        MEM_REPLACE_PLACEHOLDER, PAGE_READONLY, NULL, 0);
    if (!outBlock.view.ptr) {
        FreeBlock(blockId, index);
        return false;
    }

    outBlock.header = reinterpret_cast<const BlockFileHeader*>(outBlock.view.ptr);
    if (outBlock.header->magic != BLOCK_MAGIC || outBlock.header->version != CACHE_VERSION || outBlock.header->blockId != blockId) return false;

    outBlock.entryCount = outBlock.header->entryCount;
    outBlock.entries = reinterpret_cast<const GlyphEntry*>(static_cast<const uint8_t*>(outBlock.view.ptr) + sizeof(BlockFileHeader));

    size_t payloadOffset = sizeof(BlockFileHeader) + static_cast<size_t>(outBlock.entryCount) * sizeof(GlyphEntry);
    if (outBlock.fileSize < payloadOffset) return false;

    outBlock.payload = static_cast<const uint8_t*>(outBlock.view.ptr) + payloadOffset;

    size_t maxPayload = static_cast<size_t>(outBlock.fileSize - payloadOffset);
    for (uint32_t i = 0; i < outBlock.entryCount; ++i) {
        const GlyphEntry& e = outBlock.entries[i];
        if (e.dataSize > 0) {
            if (e.dataOffset + e.dataSize > maxPayload) return false;
        }
    }

    return true;
}

MSDFCache::MappedBlock* MSDFCache::GetOrLoadMappedBlock(uint32_t blockId) {
    auto it = m_blockCache.find(blockId);
    if (it != m_blockCache.end()) {
        return it->second;
    }

    if (s_arena.nextSlotIndex >= MAX_ARENA_SLOTS) {
        FlushAllBlocks();
    }

    MappedBlock* newBlock = new MappedBlock();
    if (!LoadMappedBlock(blockId, *newBlock)) {
        delete newBlock;
        return nullptr;
    }
    m_blockCache[blockId] = newBlock;

    return newBlock;
}

int MSDFCache::FindEntryIndexInBlock(const MappedBlock* block, uint32_t codepoint) {
    if (!block || !block->entries || block->entryCount == 0) return -1;
    const GlyphEntry* begin = block->entries;
    const GlyphEntry* end = begin + block->entryCount;
    auto it = std::lower_bound(begin, end, codepoint,
        [](const GlyphEntry& a, uint32_t v) { return a.codepoint < v; });
    if (it != end && it->codepoint == codepoint) return static_cast<int>(it - begin);
    return -1;
}

bool MSDFCache::TryLoadGlyph(uint32_t codepoint, GlyphMetrics& outMetrics) {
    auto mit = m_manifest.find(codepoint);
    if (mit == m_manifest.end()) return false;

    const ManifestEntry& me = mit->second;
    MappedBlock* blockPtr = GetOrLoadMappedBlock(me.blockId);
    if (!blockPtr) return false;

    int idx = FindEntryIndexInBlock(blockPtr, codepoint);
    if (idx < 0) return false;

    const GlyphEntry& ge = blockPtr->entries[idx];
    outMetrics.width = ge.width;
    outMetrics.height = ge.height;
    outMetrics.bitmapTop = ge.bitmapTop;
    outMetrics.bitmapLeft = ge.bitmapLeft;
    outMetrics.pixelData = ge.dataSize > 0 ? blockPtr->payload + ge.dataOffset : nullptr;

    return true;
}

bool MSDFCache::StoreGlyph(GlyphMetricsToStore&& metrics) {
    if (!m_manifestLoaded) {
        if (!LoadManifest()) return false;
    }
    s_pendingWrites.push_back(std::move(metrics));
    if (s_pendingWrites.size() >= WRITE_BATCH_SIZE) {
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
            size_t reserveSize = std::min<size_t>(estimatedEntries * 13 / 10, SIZE_MAX / sizeof(ManifestEntry) / 2);
            m_manifest.reserve(reserveSize);
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

    const ManifestHeader* hdr = reinterpret_cast<const ManifestHeader*>(static_cast<void*>(view));
    if (hdr->magic == MANIFEST_MAGIC && hdr->version == CACHE_VERSION && hdr->key == m_key) {
        uint64_t expectedSize = sizeof(ManifestHeader) + static_cast<uint64_t>(hdr->entryCount) * sizeof(ManifestEntry);
        if (fsize >= expectedSize) {
            const ManifestEntry* entries = reinterpret_cast<const ManifestEntry*>(
                static_cast<const uint8_t*>(view.ptr) + sizeof(ManifestHeader));
            for (uint32_t i = 0; i < hdr->entryCount; ++i) {
                outMap.emplace(entries[i].codepoint, entries[i]);
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
    if (!isLocked && !lock.AcquireExclusive(m_cacheManifestLockPath, 5000)) return false;

    std::filesystem::path tmpManifest = m_cacheManifestPath;
    tmpManifest.replace_extension(".tmp");

    FileGuard file(CreateFileW(tmpManifest.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL));
    if (file.handle == INVALID_HANDLE_VALUE) return false;

    ManifestHeader hdr{ MANIFEST_MAGIC, CACHE_VERSION, m_key, static_cast<uint32_t>(m_manifest.size()) };
    DWORD written = 0;

    if (!WriteFile(file.handle, &hdr, sizeof(hdr), &written, NULL)) return false;

    auto& values = m_manifest.values();
    if (!values.empty()) {
        if (!WriteFile(file.handle, values.data(), static_cast<DWORD>(values.size() * sizeof(ManifestEntry)), &written, NULL)) {
            return false;
        }
    }
    FlushFileBuffers(file.handle);

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
    if (s_pendingWrites.empty()) return true;

    std::error_code ec;
    std::filesystem::create_directories(m_cacheBasePath, ec);
    if (ec) return false;

    ankerl::unordered_dense::map<uint32_t, std::vector<GlyphMetricsToStore*>> byBlock;
    for (auto& pw : s_pendingWrites) {
        byBlock[GetBlockId(pw.codepoint)].push_back(&pw);
    }
    auto newEntries = m_mEntryPool.Acquire(s_pendingWrites.size());

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
        if (!lock.AcquireExclusive(lockPath, 5000)) continue;

        blockEntries.clear();
        if (!WriteBlockFile(blockId, kv.second, blockEntries)) continue;

        newEntries.insert(newEntries.end(), blockEntries.begin(), blockEntries.end());
    }
    s_pendingWrites.clear();

    if (!newEntries.empty()) {
        ScopedFileLock lock;
        if (lock.AcquireExclusive(m_cacheManifestLockPath, 5000)) {
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
    //m_mEntryPool.TrimToMaxPerBucket(8);
    return true;
}

bool MSDFCache::WriteBlockFile(uint32_t blockId, std::vector<GlyphMetricsToStore*>& pending, std::vector<ManifestEntry>& outEntries) {
    std::filesystem::path blockPath;
    BuildBlockPath(blockId, blockPath);

    std::sort(pending.begin(), pending.end(), [](const GlyphMetricsToStore* a, const GlyphMetricsToStore* b) {
        return a->codepoint < b->codepoint;
        });
    auto cachedBlock = GetOrLoadMappedBlock(blockId);

    uint32_t oldEntriesCount = cachedBlock ? cachedBlock->entryCount : 0;
    auto mergedEntries = m_gEntryPool.Acquire(oldEntriesCount + pending.size());

    uint32_t oldIdx = 0;
    auto pendingIt = pending.begin();

    while (oldIdx < oldEntriesCount || pendingIt != pending.end()) {
        if (oldIdx < oldEntriesCount && (pendingIt == pending.end() || cachedBlock->entries[oldIdx].codepoint < (*pendingIt)->codepoint)) {
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

    uint32_t totalPayloadSize = 0;
    for (auto& ge : mergedEntries) {
        ge.dataOffset = totalPayloadSize;
        totalPayloadSize += ge.dataSize;
    }

    auto payloadBuffer = m_vecPool.AcquireSized(totalPayloadSize);

    for (auto& ge : mergedEntries) {
        const uint8_t* src = nullptr;
        auto it = std::lower_bound(pending.begin(), pending.end(), ge.codepoint,
            [](const GlyphMetricsToStore* a, uint32_t cp) { return a->codepoint < cp; });

        if (it != pending.end() && (*it)->codepoint == ge.codepoint) {
            src = (*it)->ownedPixelData.data();
        }
        else if (cachedBlock) {
            int oldIndex = FindEntryIndexInBlock(cachedBlock, ge.codepoint);
            if (oldIndex >= 0) {
                src = cachedBlock->payload + cachedBlock->entries[oldIndex].dataOffset;
            }
        }
        if (src && ge.dataSize > 0) {
            std::memcpy(payloadBuffer.data() + ge.dataOffset, src, ge.dataSize);
        }
    }

    int32_t slotIndexToFree = -1;
    for (uint32_t i = 0; i < MSDFCache::MAX_ARENA_SLOTS; ++i) {
        if (s_arena.occupied[i] && s_arena.slotMap[i].blockId == blockId) {
            slotIndexToFree = static_cast<int32_t>(i);
            break;
        }
    }

    if (slotIndexToFree != -1) {
        FreeBlock(blockId, static_cast<uint32_t>(slotIndexToFree));
        cachedBlock = nullptr;
    }
    else {
        if (cachedBlock) cachedBlock->Close();
        auto cit = m_blockCache.find(blockId);
        if (cit != m_blockCache.end()) {
            cit->second->Close();
            delete cit->second;
            m_blockCache.erase(cit);
        }
    }

    std::filesystem::path tmpPath = blockPath;
    tmpPath.replace_extension(".tmp");

    {
        FileGuard tmpFile(CreateFileW(tmpPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL));
        if (tmpFile.handle == INVALID_HANDLE_VALUE) return false;

        BlockFileHeader bHdr{ BLOCK_MAGIC, CACHE_VERSION, blockId, static_cast<uint32_t>(mergedEntries.size()) };
        size_t dataSize = sizeof(bHdr) + (mergedEntries.size() * sizeof(GlyphEntry)) + payloadBuffer.size();

        const size_t align = s_si.dwAllocationGranularity;
        size_t paddedSize = ((dataSize + align - 1) / align) * align;
        size_t paddingNeeded = paddedSize - dataSize;

        if (paddingNeeded > 0) {
            payloadBuffer.resize(payloadBuffer.size() + paddingNeeded, 0);
        }

        DWORD written;
        if (!WriteFile(tmpFile.handle, &bHdr, sizeof(bHdr), &written, NULL) ||
            written != sizeof(bHdr)) {
            return false;
        }
        if (!WriteFile(tmpFile.handle, mergedEntries.data(),
            static_cast<DWORD>(mergedEntries.size() * sizeof(GlyphEntry)), &written, NULL)) {
            return false;
        }
        if (!WriteFile(tmpFile.handle, payloadBuffer.data(),
            static_cast<DWORD>(payloadBuffer.size()), &written, NULL)) {
            return false;
        }
        FlushFileBuffers(tmpFile.handle);
    }

    m_gEntryPool.Release(std::move(mergedEntries));
    m_vecPool.Release(std::move(payloadBuffer));

    if (!MoveFileExW(tmpPath.c_str(), blockPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
        std::filesystem::path oldPath = blockPath;
        oldPath.replace_extension(".old");

        if (MoveFileExW(tmpPath.c_str(), oldPath.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            if (cachedBlock) {
                cachedBlock->isStale = true;
                cachedBlock->stalePath = oldPath;
            }
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