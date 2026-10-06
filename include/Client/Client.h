#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace client {
inline const std::span<const char* const, 9> kLocales{reinterpret_cast<const char* const*>(0x00AD2FE0), 9};
inline int32_t& current_locale_id = *reinterpret_cast<int32_t*>(0x00C5DE9C);

inline std::string getGameLocale() { return kLocales[current_locale_id]; }
}  // namespace client
