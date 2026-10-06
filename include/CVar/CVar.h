#pragma once

#include <hookkit/hook.h>

#include <cstdint>

#include "BaseTypes.h"

class CVar {
public:
    enum CVarFlags : uint16_t {
        eCvarReadOnly = 0x4,
        eCvarCheckTaint = 0x8,
        eCvarHideFromUser = 0x40,
        eCvarReadOnlyForUser = 0x100,
    };

    using Handler = int (*)(CVar* cvar, const char* prev_val, const char* new_val, void* user_data);

    HashKeyStri hash_;
    unk_t _unk_04[4];
    const char* name_;
    uint32_t field_18_;
    CVarFlags flags_;
    uint32_t field_20_;
    uint32_t field_24_;
    const char* str_;
    unk_t _unk_2C[5];
    uint32_t bool_;
    unk_t _unk_44[9];
    Handler handler_;
    void* user_data_;

    HOOKKIT_HOOK_HANDLE(init, 0x007663F0, hookkit::Conv::eCdecl, void);
    HOOKKIT_HOOK_HANDLE(get, 0x00767460, hookkit::Conv::eCdecl, CVar*, const char*);
    HOOKKIT_HOOK_HANDLE(find, 0x00767440, hookkit::Conv::eCdecl, CVar*, const char*);
    HOOKKIT_HOOK_HANDLE(reg, 0x00767FC0, hookkit::Conv::eCdecl, CVar*, const char*, const char*, unsigned, const char*,
        Handler, uint32_t, bool, void*, bool);

    HOOKKIT_HOOK(set, 0x007667B0, hookkit::Conv::eThiscall, void, CVar*, const char*, bool, bool, bool, bool);
    HOOKKIT_HOOK(setValue, 0x007668C0, hookkit::Conv::eThiscall, char, CVar*, const char*, int, int, int, int);

    template <typename T>
    int sync(const char* raw_value, T* out, T min_val, T max_val, const char* fmt) {
        if (raw_value == nullptr || out == nullptr) { return 0; }
        T requested;
        if constexpr (std::is_floating_point_v<T>) {
            requested = static_cast<T>(std::atof(raw_value));
        } else {
            requested = static_cast<T>(std::atoi(raw_value));
        }
        const T clamped = std::clamp(requested, min_val, max_val);
        *out = clamped;
        if (requested == clamped) { return 1; }

        char buf[32];
        if constexpr (std::is_floating_point_v<T>) {
            std::snprintf(buf, sizeof(buf), fmt, static_cast<double>(clamped));
        } else {
            std::snprintf(buf, sizeof(buf), fmt, clamped);
        }
        this->set(buf, true, false, false, true);
        return 0;
    }
};
