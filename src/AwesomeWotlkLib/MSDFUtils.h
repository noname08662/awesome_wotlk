#pragma once

#include <ft2build.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <system_error>
#include <utility>
#include <vector>

#include "Utils.h"

#include FT_FREETYPE_H

class ScopedFileLock {
public:
    ScopedFileLock() = default;

    ~ScopedFileLock() { release(); }

    ScopedFileLock(const ScopedFileLock&) = delete;
    ScopedFileLock& operator=(const ScopedFileLock&) = delete;
    ScopedFileLock(ScopedFileLock&&) = delete;
    ScopedFileLock& operator=(ScopedFileLock&&) = delete;

    bool acquireExclusive(const std::filesystem::path& lock_file_path, DWORD timeout_ms = 2000) {
        return acquireInternal(lock_file_path, LOCKFILE_EXCLUSIVE_LOCK, timeout_ms);
    }

    bool acquireShared(const std::filesystem::path& lock_file_path, DWORD timeout_ms = 2000) {
        return acquireInternal(lock_file_path, 0, timeout_ms);
    }

    void release() noexcept {
        if (locked_ && file_ != INVALID_HANDLE_VALUE) {
            UnlockFileEx(file_, 0, 1, 0, &overlapped_);
            CloseHandle(file_);
            file_ = INVALID_HANDLE_VALUE;
            locked_ = false;
        }
    }

private:
    bool acquireInternal(const std::filesystem::path& lock_file_path, DWORD flags, DWORD timeout_ms) {
        std::error_code ec;
        std::filesystem::create_directories(lock_file_path.parent_path(), ec);

        file_ = CreateFileW(lock_file_path.c_str(), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (file_ == INVALID_HANDLE_VALUE) { return false; }

        const ULONGLONG start = GetTickCount64();
        for (;;) {
            overlapped_ = {};
            if (LockFileEx(file_, flags | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &overlapped_) != FALSE) {
                locked_ = true;
                return true;
            }
            if (timeout_ms == 0) { break; }
            Sleep(10);
            if ((GetTickCount64() - start) >= timeout_ms) { break; }
        }
        CloseHandle(file_);
        file_ = INVALID_HANDLE_VALUE;
        return false;
    }

    HANDLE file_ = INVALID_HANDLE_VALUE;
    OVERLAPPED overlapped_{};
    bool locked_ = false;
};

template <typename T>
class VectorPool {
public:
    VectorPool() = default;

    std::vector<T> acquire(size_t minimum_capacity) {
        const int idx = bucketIndexFor(minimum_capacity);
        auto& slot = buckets_[idx];

        if (!slot.empty()) {
            auto v = std::move(slot.back());
            slot.pop_back();
            if (v.capacity() < minimum_capacity) { v.reserve(std::max(minimum_capacity, capacityForIndex(idx))); }
            return v;
        }
        std::vector<T> v;
        v.reserve(std::max(minimum_capacity, capacityForIndex(idx)));
        return v;
    }

    std::vector<T> acquireSized(size_t size, T init = 0) {
        auto v = acquire(size);
        v.resize(size, init);
        return v;
    }

    void release(std::vector<T>&& v) {
        const size_t cap = v.capacity();
        if (cap < kMinBucketSize) { return; }
        const int idx = bucketIndexFor(cap);
        if (idx >= kBuckets || buckets_[idx].size() >= kMaxVectorsPerBucket) {
            std::vector<T>().swap(v);
            return;
        }
        v.clear();
        buckets_[idx].push_back(std::move(v));
    }

    void trimAll() {
        for (auto& slot : buckets_) {
            std::vector<std::vector<T>>().swap(slot);
        }
    }

    void trimToMaxPerBucket(size_t max_per_bucket) {
        for (auto& slot : buckets_) {
            if (slot.size() > max_per_bucket) { slot.resize(max_per_bucket); }
            slot.shrink_to_fit();
        }
    }

private:
    static constexpr size_t kMinBucketSize = 64;
    static constexpr int kBuckets = 12;
    static constexpr size_t kMaxVectorsPerBucket = 100;

    static int bucketIndexFor(size_t cap) {
        if (cap <= kMinBucketSize) { return 0; }
        const int idx = std::bit_width(cap - 1) - 6;
        return std::clamp(idx, 0, kBuckets - 1);
    }

    static size_t capacityForIndex(int idx) { return kMinBucketSize << idx; }

    std::array<std::vector<std::vector<T>>, kBuckets> buckets_;
};

struct FileGuard {
    HANDLE handle = INVALID_HANDLE_VALUE;
    std::filesystem::path path;
    bool delete_on_failure = false;
    bool successful = false;

    FileGuard() = default;

    explicit FileGuard(HANDLE h) : handle(h) {}

    FileGuard(FileGuard&& other) noexcept
        : handle(other.handle),
          path(std::move(other.path)),
          delete_on_failure(other.delete_on_failure),
          successful(other.successful) {
        other.handle = INVALID_HANDLE_VALUE;
    }

    ~FileGuard() { close(); }

    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;
    FileGuard& operator=(FileGuard&&) = delete;

    HANDLE release() {
        HANDLE h = handle;
        handle = INVALID_HANDLE_VALUE;
        return h;
    }

    void close() {
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
        if (delete_on_failure && !successful && !path.empty()) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    }

    [[nodiscard]]
    bool isValid() const {
        return handle != INVALID_HANDLE_VALUE;
    }

    explicit operator HANDLE() const { return handle; }
};

struct MappingGuard {
    HANDLE handle = nullptr;

    MappingGuard() = default;

    explicit MappingGuard(HANDLE h) : handle(h) {}

    ~MappingGuard() { close(); }

    MappingGuard(const MappingGuard&) = delete;
    MappingGuard& operator=(const MappingGuard&) = delete;
    MappingGuard(MappingGuard&&) = delete;
    MappingGuard& operator=(MappingGuard&&) = delete;

    HANDLE release() {
        HANDLE h = handle;
        handle = nullptr;
        return h;
    }

    void close() {
        if (handle != nullptr) {
            CloseHandle(handle);
            handle = nullptr;
        }
    }

    [[nodiscard]]
    bool isValid() const {
        return handle != nullptr;
    }

    explicit operator HANDLE() const { return handle; }
};

struct ViewGuard {
    void* ptr = nullptr;

    ViewGuard() = default;

    explicit ViewGuard(void* p) : ptr(p) {}

    ~ViewGuard() { close(); }

    ViewGuard(const ViewGuard&) = delete;
    ViewGuard& operator=(const ViewGuard&) = delete;
    ViewGuard(ViewGuard&&) = delete;
    ViewGuard& operator=(ViewGuard&&) = delete;

    void* release() {
        void* p = ptr;
        ptr = nullptr;
        return p;
    }

    void close() {
        if (ptr != nullptr) {
            win10Api().unmap_view_of_file2(GetCurrentProcess(), ptr, MEM_PRESERVE_PLACEHOLDER);
            ptr = nullptr;
        }
    }

    explicit operator void*() const { return ptr; }
};

template <typename F>
struct FinalAction {
    F clean;

    explicit FinalAction(F f) : clean(f) {}

    ~FinalAction() {
        try {
            clean();
        } catch (...) {}
    }

    FinalAction(const FinalAction&) = delete;
    FinalAction& operator=(const FinalAction&) = delete;
    FinalAction(FinalAction&&) = delete;
    FinalAction& operator=(FinalAction&&) = delete;
};

using FontHash = uint64_t;

inline FontHash hashFont(const FT_Byte* data, FT_Long size) {
    uint64_t h = 0xcbf29ce484222325ULL;
    if (size <= 0) { return h; }
    for (const FT_Byte b : std::span(data, static_cast<size_t>(size))) {
        h ^= b;
        h *= 0x100000001b3ULL;
    }
    return h;
}

inline bool readFontFile(const std::filesystem::path& path, std::vector<uint8_t>& out) {
    out.clear();
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { return false; }
    const std::streamsize size = file.tellg();
    if (size <= 0 || static_cast<uint64_t>(size) > static_cast<uint64_t>(LONG_MAX)) { return false; }
    out.resize(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);
    return static_cast<bool>(file.read(reinterpret_cast<char*>(out.data()), size));
}
