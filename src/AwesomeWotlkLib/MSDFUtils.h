#pragma once
#include <array>
#include <filesystem>
#include <chrono>
#include <thread>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_BBOX_H
#include FT_OUTLINE_H

class ScopedFileLock {
    HANDLE hFile = INVALID_HANDLE_VALUE;
    OVERLAPPED ol{};
    bool locked = false;
public:
    ScopedFileLock() = default;
    ~ScopedFileLock() { Release(); }
    bool AcquireExclusive(const std::filesystem::path& lockFilePath, DWORD timeoutMs = 2000) {
        return AcquireInternal(lockFilePath, LOCKFILE_EXCLUSIVE_LOCK, timeoutMs);
    }
    bool AcquireShared(const std::filesystem::path& lockFilePath, DWORD timeoutMs = 2000) {
        return AcquireInternal(lockFilePath, 0, timeoutMs);
    }
    void Release() noexcept {
        if (locked && hFile != INVALID_HANDLE_VALUE) {
            UnlockFileEx(hFile, 0, 1, 0, &ol);
            CloseHandle(hFile);
            hFile = INVALID_HANDLE_VALUE;
            locked = false;
        }
    }
private:
    bool AcquireInternal(const std::filesystem::path& lockFilePath, DWORD flags, DWORD timeoutMs) {
        std::error_code ec;
        std::filesystem::create_directories(lockFilePath.parent_path(), ec);

        hFile = CreateFileW(lockFilePath.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) return false;

        ULONGLONG start = GetTickCount64();
        do {
            memset(&ol, 0, sizeof(ol));
            if (LockFileEx(hFile, flags | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ol)) {
                locked = true;
                return true;
            }
            if (timeoutMs == 0) break;
            Sleep(10);
        } while ((GetTickCount64() - start) < timeoutMs);
        CloseHandle(hFile);
        hFile = INVALID_HANDLE_VALUE;
        return false;
    }
};

template <typename T>
class VectorPool {
    static constexpr size_t MIN_BUCKET_SIZE = 64;
    static constexpr int BUCKETS = 12;
    static constexpr size_t MAX_VECTORS_PER_BUCKET = 100;
    std::array<std::vector<std::vector<T>>, BUCKETS> buckets;

    static int bucketIndexFor(size_t cap) {
        if (cap <= MIN_BUCKET_SIZE) return 0;
        int idx = std::bit_width(cap - 1) - 6;
        return std::clamp(idx, 0, BUCKETS - 1);
    }
    static size_t capacityForIndex(int idx) {
        return MIN_BUCKET_SIZE << idx;
    }

public:
    VectorPool() = default;

    std::vector<T> Acquire(size_t minimumCapacity) {
        int idx = bucketIndexFor(minimumCapacity);
        auto& slot = buckets[idx];

        if (!slot.empty()) {
            auto v = std::move(slot.back());
            slot.pop_back();
            if (v.capacity() < minimumCapacity) {
                v.reserve(std::max(minimumCapacity, capacityForIndex(idx)));
            }
            return v;
        }
        std::vector<T> v;
        v.reserve(std::max(minimumCapacity, capacityForIndex(idx)));
        return v;
    }
    std::vector<T> AcquireSized(size_t size, T init = 0) {
        auto v = Acquire(size);
        v.resize(size, init);
        return v;
    }
    void Release(std::vector<T>&& v) {
        size_t cap = v.capacity();
        if (cap < MIN_BUCKET_SIZE) return;
        int idx = bucketIndexFor(cap);
        if (idx >= BUCKETS || buckets[idx].size() >= MAX_VECTORS_PER_BUCKET) {
            std::vector<T>().swap(v); return;
        }
        v.clear();
        buckets[idx].push_back(std::move(v));
    }
    void TrimAll() {
        for (auto& slot : buckets) {
            std::vector<std::vector<T>>().swap(slot);
        }
    }
    void TrimToMaxPerBucket(size_t maxPerBucket) {
        for (auto& slot : buckets) {
            if (slot.size() > maxPerBucket) {
                slot.resize(maxPerBucket);
            }
            slot.shrink_to_fit();
        }
    }
};

struct FileGuard {
    HANDLE handle = INVALID_HANDLE_VALUE;
    std::filesystem::path path;
    bool deleteOnFailure = false;
    bool successful = false;

    FileGuard(HANDLE h = INVALID_HANDLE_VALUE) : handle(h) {}
    FileGuard(FileGuard&& other) noexcept
        : handle(other.handle), path(std::move(other.path)),
        deleteOnFailure(other.deleteOnFailure), successful(other.successful) {
        other.handle = INVALID_HANDLE_VALUE;
    }
    ~FileGuard() { Close(); }
    HANDLE Release() { HANDLE h = handle; handle = INVALID_HANDLE_VALUE; return h; }
    void Close() {
        if (handle != INVALID_HANDLE_VALUE) {
            CloseHandle(handle);
            handle = INVALID_HANDLE_VALUE;
        }
        if (deleteOnFailure && !successful && !path.empty()) {
            std::error_code ec;
            std::filesystem::remove(path, ec);
        }
    }
    bool IsValid() const { return handle != INVALID_HANDLE_VALUE; }
    operator HANDLE() const { return handle; }

    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;
};

struct MappingGuard {
    HANDLE handle = NULL;
    MappingGuard(HANDLE h = NULL) : handle(h) {}
    ~MappingGuard() { Close(); }
    HANDLE Release() { HANDLE h = handle; handle = NULL; return h; }
    void Close() {
        if (handle != NULL) {
            CloseHandle(handle);
            handle = NULL;
        }
    }
    operator HANDLE() const { return handle; }
    bool IsValid() const { return handle != NULL; }
    MappingGuard(const MappingGuard&) = delete;
    MappingGuard& operator=(const MappingGuard&) = delete;
};

struct ViewGuard {
    void* ptr = nullptr;
    ViewGuard(void* p = nullptr) : ptr(p) {}
    ~ViewGuard() { Close(); }
    void* Release() { void* p = ptr; ptr = nullptr; return p; }
    void Close() {
        if (ptr) {
            UnmapViewOfFile2(GetCurrentProcess(), ptr, MEM_PRESERVE_PLACEHOLDER);
            ptr = nullptr;
        }
    }
    operator void* () const { return ptr; }
    ViewGuard(const ViewGuard&) = delete;
    ViewGuard& operator=(const ViewGuard&) = delete;
};

class Throttle {
private:
    double targetUsage;
    std::chrono::steady_clock::time_point lastSleep;
    std::chrono::milliseconds accumulatedWork{ 0 };

public:
    Throttle(double targetPercent)
        : targetUsage(std::clamp(targetPercent, 1.0, 100.0)),
        lastSleep(std::chrono::steady_clock::now()) {
    }
    void StartWork() {
        lastSleep = std::chrono::steady_clock::now();
    }
    void EndWork() {
        auto now = std::chrono::steady_clock::now();
        auto workDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSleep);
        accumulatedWork += workDuration;

        if (accumulatedWork.count() >= 100) {
            int sleepMs = static_cast<int>(accumulatedWork.count() / (targetUsage / 100.0) - accumulatedWork.count());
            if (sleepMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            }
            accumulatedWork = std::chrono::milliseconds{ 0 };
        }
        lastSleep = std::chrono::steady_clock::now();
    }
};

struct ConsoleGuard {
    FILE* fpOut = nullptr;
    FILE* fpIn = nullptr;
    bool allocated = false;
    HWND wnd = nullptr;

    ConsoleGuard() {
        wnd = GetActiveWindow();
        allocated = AllocConsole() || GetLastError() == ERROR_ACCESS_DENIED;
        if (allocated) {
            freopen_s(&fpOut, "CONOUT$", "w", stdout);
            freopen_s(&fpIn, "CONIN$", "r", stdin);
            if (wnd) {
                ShowWindow(wnd, SW_MINIMIZE);
            }
            SetForegroundWindow(GetConsoleWindow());
        }
    }
    ~ConsoleGuard() {
        if (fpOut) fclose(fpOut);
        if (fpIn) fclose(fpIn);
        if (allocated) FreeConsole();
        if (wnd) {
            ShowWindow(wnd, SW_RESTORE);
            SetForegroundWindow(wnd);
        }
    }
};

template <typename F>
struct FinalAction {
    F clean;
    FinalAction(F f) : clean(f) {}
    ~FinalAction() { clean(); }
    FinalAction(const FinalAction&) = delete;
    FinalAction& operator=(const FinalAction&) = delete;
};

using FontHash = uint64_t;
static FontHash HashFont(const FT_Byte* data, FT_Long size) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (FT_Long i = 0; i < size; ++i) {
        h ^= data[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}
