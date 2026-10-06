#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "desc.h"
#include "hook.h"

namespace hookkit {

template <std::uintptr_t... Addrs>
struct Targets {
    static constexpr std::size_t kSize = sizeof...(Addrs);
    static constexpr std::array<std::uintptr_t, kSize> kData{Addrs...};

    template <std::size_t Index>
    static constexpr std::uintptr_t get() {
        static_assert(Index < kSize,
            "hookkit::Targets::get<Index>: Branch target index out of range -- Index must be < the number of addresses "
            "listed in Targets<...>");
        return kData.data()[Index];
    }
};

template <auto Key, std::uintptr_t Addr>
struct Branch {
    static constexpr auto kKey = Key;
    static constexpr std::uintptr_t kAddress = Addr;
};

template <typename... Branches>
struct BranchMap {
    static constexpr std::size_t kSize = sizeof...(Branches);

    static constexpr std::array<std::uintptr_t, kSize> kData{Branches::kAddress...};

    template <std::size_t Index>
    static constexpr std::uintptr_t getByIndex() {
        static_assert(
            Index < kSize, "hookkit::BranchMap: Index out of range -- Index must be < the number of Branch<> entries");
        return kData.data()[Index];
    }

    template <auto Key>
    static constexpr std::uintptr_t getByKey() {
        constexpr std::size_t kIdx = findIndex<Key>();
        static_assert(kIdx < kSize,
            "hookkit: Key not found in BranchMap -- no Branch<Key, Addr> entry has this key (a key of another type, "
            "e.g. a different enum, never matches)");
        return kData.data()[kIdx];
    }

    template <std::size_t Index>
    static constexpr std::uintptr_t get() {
        static_assert(
            Index < kSize, "hookkit::BranchMap: Index out of range -- Index must be < the number of Branch<> entries");
        return kData.data()[Index];
    }

private:
    template <auto Key, typename B>
    static constexpr bool checkMatch() {
        using LKey = std::remove_cv_t<decltype(B::kKey)>;
        using RKey = std::remove_cv_t<decltype(Key)>;
        if constexpr (std::is_same_v<LKey, RKey>) {
            return B::kKey == Key;
        } else {
            return false;
        }
    }

    template <auto Key>
    static constexpr std::size_t findIndex() {
        if constexpr (kSize == 0) {
            return 0;
        } else {
            std::array<bool, kSize> matches{checkMatch<Key, Branches>()...};

            for (std::size_t i = 0; i < kSize; ++i) {
                if (matches[i]) { return i; }
            }
            return kSize;
        }
    }
};

template <typename Tag, std::uintptr_t Addr, typename TargetRegistry, Conv Abi, typename Ret, typename... Params>
struct SiteHook : Hook<Tag, Addr, Abi, Ret, Params...> {
    using Targets = TargetRegistry;

    static constexpr std::size_t kTargetCount = TargetRegistry::kSize;

    static constexpr auto kTargets = TargetRegistry::kData;

    template <std::size_t Idx>
    static constexpr std::uintptr_t targetByIndex() {
        return TargetRegistry::template get<Idx>();
    }

    template <auto Key>
    static constexpr std::uintptr_t targetByKey() {
        return TargetRegistry::template getByKey<Key>();
    }

    template <std::size_t Idx>
    static constexpr std::uintptr_t target() {
        return TargetRegistry::template get<Idx>();
    }

    static constexpr std::uintptr_t targetAt(std::size_t idx) {
        assert(idx < kTargetCount &&
            "hookkit: SiteHook::targetAt: index out of range -- idx must be < kTargetCount (target<Idx>() checks "
            "this at compile time)");
        return TargetRegistry::kData.data()[idx];
    }
};

/**
 * @brief Defines a naked SiteHook struct handle (used for hooking multi-branch endpoints).
 * @param NAME The name of the struct handle.
 * @param ADDR The target memory address.
 * @param TARGETS_SPEC The target registry (e.g., Targets or BranchMap).
 */
#define HOOKKIT_SITE_HOOK_HANDLE(NAME, ADDR, TARGETS_SPEC)                                                 \
    struct NAME##_tag {};                                                                                  \
    struct NAME : ::hookkit::SiteHook<NAME##_tag, (ADDR), TARGETS_SPEC, ::hookkit::Conv::eCdecl, void> {}; \
    using NAME##_hook = NAME;                                                                              \
    static_assert(std::is_empty_v<NAME>, #NAME " must stay zero-sized" HOOKKIT_ZERO_SIZED_WHY_)

}  // namespace hookkit
