#include "MSDFManager.h"
#include "MSDFCache.h"

#pragma comment(lib, "onecore.lib")

MSDFManager::MSDFManager() {
    GetSystemInfo(&s_si);
}

MSDFManager::~MSDFManager() {
    FlushAll();
}

void MSDFManager::MappedBlock::Close() {
    view.Close();
    mapping.Close();
    file.Close();
    header = nullptr;
    entries = nullptr;
    hashTable = nullptr;
    payload = nullptr;
    entryCount = 0;
}

MSDFManager::ArenaState::ArenaState() {
    if (!MSDF::IS_WIN10) {
        base = nullptr;
        return;
    }

    const uint32_t maxGlyphDim = MSDF::SDF_RENDER_SIZE + 2 * MSDF::SDF_SPREAD;
    const uint32_t maxPixelsPerGlyph = maxGlyphDim * maxGlyphDim;
    const uint32_t maxBytesPerGlyph = maxPixelsPerGlyph * 4;

    const size_t maxPayload = MSDFCache::BLOCK_SIZE * maxBytesPerGlyph;
    const size_t maxEntries = MSDFCache::BLOCK_SIZE * sizeof(MSDFCache::GlyphEntry);
    const size_t maxHashTable = MSDFCache::BLOCK_SIZE * sizeof(uint32_t);
    const size_t maxBlockSize = sizeof(MSDFCache::BlockFileHeader) + maxEntries + maxHashTable + maxPayload;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const size_t gran = si.dwAllocationGranularity;
    effectiveSlotSize = ((maxBlockSize + gran - 1) / gran) * gran;

    slotToBlockIndex.fill(0xFFFFFFFF);

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

MSDFManager::ArenaState::~ArenaState() {
    if (base) {
        VirtualFreeEx(GetCurrentProcess(), base, 0, MEM_RELEASE);
        base = nullptr;
    }
}

void* MSDFManager::ArenaState::GetFreeSlot(uint32_t blockIndex, uint32_t& outSlotIndex) {
    if (freeMask == 0) return nullptr;
    uint32_t slotIdx = static_cast<uint32_t>(std::countr_zero(freeMask));
    freeMask &= ~(1ULL << slotIdx);
    slotToBlockIndex[slotIdx] = blockIndex;
    outSlotIndex = slotIdx;
    return slotAddresses[slotIdx];
}

void MSDFManager::ArenaState::FreeSlot(uint32_t slotIndex) {
    if (slotIndex >= MAX_ARENA_SLOTS || !IsSlotOccupied(slotIndex)) return;

    void* slotAddr = slotAddresses[slotIndex];
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

    freeMask |= (1ULL << slotIndex);

    slotToBlockIndex[slotIndex] = 0xFFFFFFFF;
}

void MSDFManager::ArenaState::FlushAll() {
    for (uint32_t i = 0; i < MAX_ARENA_SLOTS; ++i) {
        if (IsSlotOccupied(i)) FreeSlot(i);
    }
}

void MSDFManager::FreeBlock(uint32_t blockIndex) {
    if (blockIndex >= MAX_ARENA_SLOTS) return;

    MappedBlock& block = s_mappedBlocks[blockIndex];
    MSDFCache::BlockKey keyToErase = block.key;

    if (block.slotIndex != 0xFFFFFFFF) {
        s_arena.FreeSlot(block.slotIndex);
    }
    block.Reset();
    s_blockCache.erase(keyToErase);

    if (s_lastBlockIndex == blockIndex) {
        s_lastBlockIndex = 0xFFFFFFFF;
        s_lastBlockKey = {};
    }
}

void MSDFManager::FreeBlockByKey(MSDFCache::BlockKey key) {
    auto it = s_blockCache.find(key);
    if (it == s_blockCache.end()) return;
    FreeBlock(it->second);
}

void MSDFManager::FlushAll() {
    for (auto& block : s_mappedBlocks) {
        block.Reset();
    }
    s_blockCache.clear();
    s_arena.FlushAll();

    s_lastBlockIndex = 0xFFFFFFFF;
    s_lastBlockKey = {};
}

uint32_t MSDFManager::RegisterFont(FontHash hash) {
    auto it = s_fontHashToId.find(hash);
    if (it != s_fontHashToId.end()) {
        return it->second;
    }

    uint32_t fontId = s_nextFontId++;
    s_fontHashToId.emplace(hash, fontId);
    s_fontIdToHash.emplace(fontId, hash);
    return fontId;
}

FontHash MSDFManager::GetFontHash(uint32_t fontId) const {
    auto it = s_fontIdToHash.find(fontId);
    return (it != s_fontIdToHash.end()) ? it->second : 0;
}

bool MSDFManager::LoadMappedBlock(MSDFCache::BlockWrap wrap, MappedBlock& outBlock, void* slotAddr, uint32_t slotIndex) {
    outBlock.file.handle = CreateFileW(wrap.path.native().c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL);
    if (outBlock.file.handle == INVALID_HANDLE_VALUE) {
        s_arena.FreeSlot(slotIndex);
        return false;
    }

    LARGE_INTEGER fileSizeLI;
    if (!GetFileSizeEx(outBlock.file.handle, &fileSizeLI)) {
        s_arena.FreeSlot(slotIndex);
        return false;
    }
    outBlock.fileSize = static_cast<uint64_t>(fileSizeLI.QuadPart);

    const size_t allocGran = s_si.dwAllocationGranularity;
    size_t splitSize = ((outBlock.fileSize + allocGran - 1) / allocGran) * allocGran;
    if (splitSize < allocGran) splitSize = allocGran;
    if (splitSize > s_arena.SlotSize()) {
        s_arena.FreeSlot(slotIndex);
        return false;
    }
    outBlock.slotIndex = slotIndex;

    if (!VirtualFreeEx(GetCurrentProcess(), slotAddr, splitSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) {
        s_arena.FreeSlot(slotIndex);
        return false;
    }

    DWORD sizeHigh = static_cast<DWORD>(splitSize >> 32);
    DWORD sizeLow = static_cast<DWORD>(splitSize & 0xFFFFFFFF);
    outBlock.mapping.handle = CreateFileMappingW(outBlock.file.handle, NULL, PAGE_READONLY, sizeHigh, sizeLow, NULL);
    if (!outBlock.mapping.handle) {
        s_arena.FreeSlot(slotIndex);
        return false;
    }

    outBlock.view.ptr = MapViewOfFile3(outBlock.mapping.handle, NULL, slotAddr, 0, splitSize,
        MEM_REPLACE_PLACEHOLDER, PAGE_READONLY, NULL, 0);
    if (!outBlock.view.ptr) {
        s_arena.FreeSlot(slotIndex);
        return false;
    }

    outBlock.header = reinterpret_cast<const MSDFCache::BlockFileHeader*>(outBlock.view.ptr);
    if (outBlock.header->magic != MSDFCache::BLOCK_MAGIC ||
        outBlock.header->version != MSDFCache::CACHE_VERSION ||
        outBlock.header->blockId != wrap.key.blockId ||
        outBlock.header->entryCount > MSDFCache::BLOCK_SIZE) {
        s_arena.FreeSlot(slotIndex);
        return false;
    }

    outBlock.entryCount = outBlock.header->entryCount;
    outBlock.entries = reinterpret_cast<const MSDFCache::GlyphEntry*>(
        static_cast<const uint8_t*>(outBlock.view.ptr) + sizeof(MSDFCache::BlockFileHeader));

    size_t hashTableOffset = sizeof(MSDFCache::BlockFileHeader) +
        static_cast<size_t>(outBlock.entryCount) * sizeof(MSDFCache::GlyphEntry);
    outBlock.hashTable = reinterpret_cast<const uint32_t*>(
        static_cast<const uint8_t*>(outBlock.view.ptr) + hashTableOffset);

    size_t payloadOffset = hashTableOffset + (MSDFCache::BLOCK_SIZE * sizeof(uint32_t));
    if (outBlock.fileSize < payloadOffset) {
        s_arena.FreeSlot(slotIndex);
        return false;
    }
    outBlock.payload = static_cast<const uint8_t*>(outBlock.view.ptr) + payloadOffset;

    size_t maxPayload = static_cast<size_t>(outBlock.fileSize - payloadOffset);
    for (uint32_t i = 0; i < outBlock.entryCount; ++i) {
        const MSDFCache::GlyphEntry& e = outBlock.entries[i];
        if (e.dataSize > 0) {
            if (e.dataOffset + e.dataSize > maxPayload) {
                s_arena.FreeSlot(slotIndex);
                return false;
            }
        }
    }
    outBlock.key = wrap.key;

    return true;
}

MSDFManager::MappedBlock* MSDFManager::GetOrLoadMappedBlock(MSDFCache::BlockWrap wrap) {
    if (s_lastBlockIndex != 0xFFFFFFFF && s_lastBlockKey == wrap.key) {
        return &s_mappedBlocks[s_lastBlockIndex];
    }

    auto it = s_blockCache.find(wrap.key);
    if (it != s_blockCache.end()) {
        s_lastBlockIndex = it->second;
        s_lastBlockKey = wrap.key;
        return &s_mappedBlocks[it->second];
    }
    if (s_arena.freeMask == 0) FlushAll();

    uint32_t slotIndex = 0;
    void* slotAddr = s_arena.GetFreeSlot(wrap.key.blockId, slotIndex);
    if (slotAddr == nullptr) return nullptr;

    MappedBlock& newBlock = s_mappedBlocks[slotIndex];
    if (!LoadMappedBlock(wrap, newBlock, slotAddr, slotIndex)) return nullptr;

    s_lastBlockIndex = slotIndex;
    s_lastBlockKey = wrap.key;

    s_blockCache[wrap.key] = slotIndex;
    return &newBlock;
}

bool MSDFManager::LoadGlyph(MSDFCache::BlockWrap wrap, uint32_t codepoint, GlyphMetrics& outMetrics) {
    MappedBlock* blockPtr = GetOrLoadMappedBlock(wrap);
    if (!blockPtr) return false;

    uint32_t entryIndex = blockPtr->hashTable[codepoint & (MSDFCache::BLOCK_SIZE - 1)];
    if (entryIndex == 0xFFFFFFFF || entryIndex >= blockPtr->entryCount) return false;

    const MSDFCache::GlyphEntry& ge = blockPtr->entries[entryIndex];
    if (ge.codepoint != codepoint) return false;

    outMetrics.width = ge.width;
    outMetrics.height = ge.height;
    outMetrics.bitmapTop = ge.bitmapTop;
    outMetrics.bitmapLeft = ge.bitmapLeft;
    outMetrics.pixelData = ge.dataSize > 0 ? blockPtr->payload + ge.dataOffset : nullptr;

    return true;
}