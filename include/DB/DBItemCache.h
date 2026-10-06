#pragma once

#include <hookkit/hook.h>

#include <cstdint>

#include "BaseTypes.h"

class DbItemCache {
public:
    HOOKKIT_HOOK(getItemInfoBlockById, 0x0067CA30, hookkit::Conv::eThiscall, uintptr_t, DbItemCache*, uint32_t, guid_t*,
        int, int, int);
    inline static const auto kWdbCacheItem = reinterpret_cast<DbItemCache*>(0x00C5D828);
};
