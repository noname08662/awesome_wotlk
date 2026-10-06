#include "MSDFManager.h"

#include <algorithm>
#include <bit>
#include <span>

#include "MSDF.h"
#include "MSDFCache.h"
#include "Utils.h"

MSDFManager::MappedBlock::MappedBlock() = default;

void MSDFManager::MappedBlock::close() {
    view.close();
    mapping.close();
    file.close();
    header = nullptr;
    entries = {};
    hash_table = {};
    payload = {};
}

MSDFManager::ArenaState::ArenaState() {
    if (!isWin10()) { return; }

    constexpr uint32_t kMaxGlyphDim = msdf::kSdfRenderSize + 2 * msdf::kSdfSpread;
    constexpr uint32_t kMaxPixelsPerGlyph = kMaxGlyphDim * kMaxGlyphDim;
    constexpr uint32_t kMaxBytesPerGlyph = kMaxPixelsPerGlyph * 4;

    constexpr size_t kMaxPayload = MSDFCache::kBlockSize * kMaxBytesPerGlyph;
    constexpr size_t kMaxEntries = MSDFCache::kBlockSize * sizeof(MSDFCache::GlyphEntry);
    constexpr size_t kMaxHashTable = MSDFCache::kBlockSize * sizeof(uint32_t);
    constexpr size_t kMaxBlockSize = sizeof(MSDFCache::BlockFileHeader) + kMaxEntries + kMaxHashTable + kMaxPayload;

    SYSTEM_INFO si;
    GetSystemInfo(&si);
    const size_t gran = si.dwAllocationGranularity;
    effective_slot_size_ = ((kMaxBlockSize + gran - 1) / gran) * gran;

    slot_to_block_index_.fill(kInvalidIndex);

    const size_t total_size = effective_slot_size_ * kMaxArenaSlots;
    void* base = win10Api().virtual_alloc2(
        GetCurrentProcess(), nullptr, total_size, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
    if (base == nullptr) { return; }

    const std::span arena(static_cast<std::byte*>(base), total_size);
    for (size_t i = 0; i < kMaxArenaSlots; ++i) {
        void* slot_addr = arena.subspan(i * effective_slot_size_).data();
        slot_addresses_[i] = slot_addr;
        VirtualFreeEx(GetCurrentProcess(), slot_addr, effective_slot_size_, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER);
    }
}

void* MSDFManager::ArenaState::getFreeSlot(uint32_t block_index, uint32_t& out_slot_index) {
    if (free_mask_ == 0 || slot_addresses_[0] == nullptr) { return nullptr; }
    const auto slot_idx = static_cast<uint32_t>(std::countr_zero(free_mask_));
    free_mask_ &= ~(1ULL << slot_idx);
    slot_to_block_index_[slot_idx] = block_index;
    out_slot_index = slot_idx;
    return slot_addresses_[slot_idx];
}

void MSDFManager::ArenaState::freeSlot(uint32_t slot_index) {
    if (slot_index >= kMaxArenaSlots || !isSlotOccupied(slot_index)) { return; }

    void* slot_addr = slot_addresses_[slot_index];
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(slot_addr, &mbi, sizeof(mbi)) != 0 && mbi.RegionSize < effective_slot_size_) {
        VirtualFreeEx(GetCurrentProcess(), slot_addr, effective_slot_size_, MEM_RELEASE | MEM_COALESCE_PLACEHOLDERS);
    }

    free_mask_ |= (1ULL << slot_index);

    slot_to_block_index_[slot_index] = kInvalidIndex;
}

void MSDFManager::ArenaState::flushAll() {
    for (uint32_t i = 0; i < kMaxArenaSlots; ++i) {
        if (isSlotOccupied(i)) { freeSlot(i); }
    }
}

bool MSDFManager::loadGlyph(const MSDFCache::BlockWrap& wrap, uint32_t codepoint, GlyphMetrics& out_metrics) {
    const MappedBlock* block = getOrLoadMappedBlock(wrap);
    if (block == nullptr) { return false; }

    const uint32_t entry_index = block->hash_table[codepoint & (MSDFCache::kBlockSize - 1)];
    if (entry_index == kInvalidIndex || entry_index >= block->entries.size()) { return false; }

    const MSDFCache::GlyphEntry& ge = block->entries[entry_index];
    if (ge.codepoint != codepoint) { return false; }

    out_metrics.width = ge.width;
    out_metrics.height = ge.height;
    out_metrics.bitmap_top = ge.bitmap_top;
    out_metrics.bitmap_left = ge.bitmap_left;
    out_metrics.pixel_data = ge.data_size > 0 ? block->payload.subspan(ge.data_offset).data() : nullptr;

    return true;
}

void MSDFManager::prefetchGlyphs(const MSDFCache::BlockWrap& wrap, std::span<const uint32_t> codepoints) {
    const auto prefetch = win10Api().prefetch_virtual_memory;
    if (prefetch == nullptr) { return; }
    const MappedBlock* block = getOrLoadMappedBlock(wrap);
    if (block == nullptr) { return; }

    std::array<WIN32_MEMORY_RANGE_ENTRY, MSDFCache::kBlockSize> ranges;
    size_t count = 0;
    for (const uint32_t codepoint : codepoints) {
        const uint32_t entry_index = block->hash_table[codepoint & (MSDFCache::kBlockSize - 1)];
        if (entry_index == kInvalidIndex || entry_index >= block->entries.size()) { continue; }
        const MSDFCache::GlyphEntry& ge = block->entries[entry_index];
        if (ge.codepoint != codepoint || ge.data_size == 0 || count == ranges.size()) { continue; }
        ranges[count++] = {
            .VirtualAddress = const_cast<uint8_t*>(block->payload.subspan(ge.data_offset).data()),
            .NumberOfBytes = ge.data_size
        };
    }
    if (count != 0) { prefetch(GetCurrentProcess(), count, ranges.data(), 0); }
}

bool MSDFManager::loadMappedBlock(
    const MSDFCache::BlockWrap& wrap, MappedBlock& out_block, void* slot_addr, uint32_t slot_index) {
    const auto fail = [&out_block, slot_index] {
        out_block.reset();
        kArena->freeSlot(slot_index);
        return false;
    };

    out_block.file.handle =
        CreateFileW(wrap.path.native().c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, nullptr);
    if (out_block.file.handle == INVALID_HANDLE_VALUE) { return fail(); }

    LARGE_INTEGER file_size_li;
    if (GetFileSizeEx(out_block.file.handle, &file_size_li) == 0) { return fail(); }
    out_block.file_size = static_cast<uint64_t>(file_size_li.QuadPart);

    const size_t alloc_gran = kSystemInfo->dwAllocationGranularity;
    const uint64_t split_size =
        std::max<uint64_t>(((out_block.file_size + alloc_gran - 1) / alloc_gran) * alloc_gran, alloc_gran);
    if (split_size > kArena->slotSize()) { return fail(); }
    out_block.slot_index = slot_index;

    if (split_size < kArena->slotSize() &&
        VirtualFreeEx(GetCurrentProcess(), slot_addr, split_size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER) == 0) {
        return fail();
    }

    const auto size_high = static_cast<DWORD>(split_size >> 32);
    const auto size_low = static_cast<DWORD>(split_size & 0xFFFFFFFF);
    out_block.mapping.handle =
        CreateFileMappingW(out_block.file.handle, nullptr, PAGE_READONLY, size_high, size_low, nullptr);
    if (out_block.mapping.handle == nullptr) { return fail(); }

    out_block.view.ptr = win10Api().map_view_of_file3(out_block.mapping.handle, nullptr, slot_addr, 0, split_size,
        MEM_REPLACE_PLACEHOLDER, PAGE_READONLY, nullptr, 0);
    if (out_block.view.ptr == nullptr) { return fail(); }

    out_block.header = static_cast<const MSDFCache::BlockFileHeader*>(out_block.view.ptr);
    if (out_block.header->magic != MSDFCache::kBlockMagic || out_block.header->version != MSDFCache::kCacheVersion ||
        out_block.header->block_id != wrap.key.block_id || out_block.header->entry_count > MSDFCache::kBlockSize) {
        return fail();
    }

    const uint32_t entry_count = out_block.header->entry_count;
    constexpr size_t kEntriesOffset = sizeof(MSDFCache::BlockFileHeader);
    const size_t hash_table_offset = kEntriesOffset + entry_count * sizeof(MSDFCache::GlyphEntry);
    const size_t payload_offset = hash_table_offset + (MSDFCache::kBlockSize * sizeof(uint32_t));
    if (out_block.file_size < payload_offset) { return fail(); }

    const std::span view_bytes(
        static_cast<const uint8_t*>(out_block.view.ptr), static_cast<size_t>(out_block.file_size));
    out_block.entries = {
        reinterpret_cast<const MSDFCache::GlyphEntry*>(view_bytes.subspan(kEntriesOffset).data()), entry_count};
    out_block.hash_table = {
        reinterpret_cast<const uint32_t*>(view_bytes.subspan(hash_table_offset).data()), MSDFCache::kBlockSize};
    out_block.payload = view_bytes.subspan(payload_offset);

    for (const MSDFCache::GlyphEntry& e : out_block.entries) {
        if (e.data_size > 0 && static_cast<uint64_t>(e.data_offset) + e.data_size > out_block.payload.size()) {
            return fail();
        }
    }
    out_block.key = wrap.key;

    return true;
}

MSDFManager::MappedBlock* MSDFManager::getOrLoadMappedBlock(const MSDFCache::BlockWrap& wrap) {
    auto& blocks = *kMappedBlocks;
    auto& last_block_key = *kLastBlockKey;
    if (last_block_index_ != kInvalidIndex && last_block_key == wrap.key) { return &blocks[last_block_index_]; }

    auto& block_cache = *kBlockCache;
    const auto it = block_cache.find(wrap.key);
    if (it != block_cache.end()) {
        last_block_index_ = it->second;
        last_block_key = wrap.key;
        return &blocks[it->second];
    }
    if (kArena->isFull()) { flushAll(); }

    uint32_t slot_index = 0;
    void* slot_addr = kArena->getFreeSlot(wrap.key.block_id, slot_index);
    if (slot_addr == nullptr) { return nullptr; }

    MappedBlock& new_block = blocks[slot_index];
    if (!loadMappedBlock(wrap, new_block, slot_addr, slot_index)) { return nullptr; }

    last_block_index_ = slot_index;
    last_block_key = wrap.key;

    block_cache[wrap.key] = slot_index;
    return &new_block;
}

void MSDFManager::freeBlock(uint32_t block_index) {
    if (block_index >= kMaxArenaSlots) { return; }

    MappedBlock& block = kMappedBlocks[block_index];
    const MSDFCache::BlockKey key_to_erase = block.key;

    const uint32_t slot_index = block.slot_index;
    block.reset();
    if (slot_index != kInvalidIndex) { kArena->freeSlot(slot_index); }
    kBlockCache->erase(key_to_erase);

    if (last_block_index_ == block_index) {
        last_block_index_ = kInvalidIndex;
        *kLastBlockKey = {};
    }
}

void MSDFManager::freeBlockByKey(MSDFCache::BlockKey key) {
    const auto it = kBlockCache->find(key);
    if (it == kBlockCache->end()) { return; }
    freeBlock(it->second);
}

void MSDFManager::flushAll() {
    for (auto& block : *kMappedBlocks) {
        block.reset();
    }
    kBlockCache->clear();
    kArena->flushAll();

    last_block_index_ = kInvalidIndex;
    *kLastBlockKey = {};
}

uint32_t MSDFManager::registerFont(FontHash hash) {
    const auto it = kFontHashToId->find(hash);
    if (it != kFontHashToId->end()) { return it->second; }

    const uint32_t font_id = next_font_id_++;
    kFontHashToId->emplace(hash, font_id);
    kFontIdToHash->emplace(font_id, hash);
    return font_id;
}

FontHash MSDFManager::getFontHash(uint32_t font_id) {
    const auto it = kFontIdToHash->find(font_id);
    return it != kFontIdToHash->end() ? it->second : 0;
}
