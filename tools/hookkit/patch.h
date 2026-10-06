#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX

#endif
#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace hookkit {
class ScopedUnprotect {
public:
    ScopedUnprotect(void* address, std::size_t size)
        : address_(address),
          size_(size),
          ok_(VirtualProtect(address_, size_, PAGE_EXECUTE_READWRITE, &old_protect_) != 0) {}

    ~ScopedUnprotect() {
        if (!ok_) { return; }

        DWORD temp = 0;
        VirtualProtect(address_, size_, old_protect_, &temp);
    }

    ScopedUnprotect(const ScopedUnprotect&) = delete;
    ScopedUnprotect& operator=(const ScopedUnprotect&) = delete;

    ScopedUnprotect(ScopedUnprotect&&) = delete;
    ScopedUnprotect& operator=(ScopedUnprotect&&) = delete;

    [[nodiscard]]
    bool ok() const noexcept {
        return ok_;
    }

private:
    void* address_;
    std::size_t size_;
    DWORD old_protect_ = 0;
    bool ok_ = false;
};

template <typename T>
bool patchBytes(void* address, const T* data, std::size_t size) {
    ScopedUnprotect guard(address, size);
    if (!guard.ok()) { return false; }

    std::memcpy(address, data, size);
    FlushInstructionCache(GetCurrentProcess(), address, size);
    return true;
}

template <typename T>
bool forceWrite(std::uintptr_t address, const T& value) {
    return patchBytes(reinterpret_cast<void*>(address), &value, sizeof(T));
}
}  // namespace hookkit
