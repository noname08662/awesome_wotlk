#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>

#include "abi_macros.h"
#include "desc.h"

namespace hookkit {

template <typename Tag, std::uintptr_t Addr, Conv Abi, typename Ret, typename... Params>
struct Hook {
    using Descriptor = Desc<Abi, Ret, Params...>;

    static_assert(Abi != Conv::eThiscall || sizeof...(Params) >= 1,
        "eThiscall hooks need an explicit leading class-pointer parameter, "
        "e.g. HOOKKIT_HOOK(Name, addr, hookkit::Conv::eThiscall, Ret, MyClass*, ...)");
    static_assert(Abi != Conv::eUsercall && Abi != Conv::eUserpurge,
        "usercall/userpurge have no compiler-generatable ABI; use "
        "hookkit::WildHook via HOOKKIT_HOOK_WILD instead of Hook/HOOKKIT_HOOK.");

    using RawFn = Descriptor::RawFn;
    using HandlerFn = Descriptor::HandlerFn;
    using ClassType = Descriptor::ClassType;
    using ReturnType = Ret;
    using Args = Descriptor::Args;

    static constexpr Conv kAbi = Abi;
    static constexpr std::uintptr_t kAddress = Addr;
    static constexpr std::size_t kArity = sizeof...(Params);
    static constexpr std::size_t kUserArity = (Abi == Conv::eThiscall && kArity > 0) ? kArity - 1 : kArity;

    inline static constinit std::atomic<std::uintptr_t> target{Addr};

    inline static std::atomic<HandlerFn> hook{nullptr};

    inline static std::atomic<bool> attached{false};

    inline static std::atomic<void*> raw_detour{nullptr};

    inline static std::atomic<std::uint32_t> in_flight{0};
    inline static std::atomic<bool> detaching{false};
    inline static detail::QuiesceSlots slots{};

    static constexpr auto kBridge = &Descriptor::template bridge<hook, target, in_flight, detaching, slots>;

    static_assert(
        sizeof(std::atomic<std::uintptr_t>) == sizeof(void*) && alignof(std::atomic<std::uintptr_t>) == alignof(void*),
        "hookkit: std::atomic<uintptr_t> must be layout-compatible with void* -- patchSlot() reads/"
        "writes it as a raw PVOID* for both the MinHook in/out idiom and the bridge's own load");

    static void* resolveDetour() {
        if (void* ptr = raw_detour.load(std::memory_order_acquire)) { return ptr; }
        return reinterpret_cast<void*>(kBridge);
    }

    static void** patchSlot() { return reinterpret_cast<void**>(&target); }

    static bool quiescent(bool exclude_caller = false) {
        if (in_flight.load(std::memory_order_seq_cst) != 0) { return false; }
        return slots.idle(exclude_caller ? detail::quiesce_thread_slot : -1);
    }

    template <const auto& Fn>
    static void* staticDetour() {
        return reinterpret_cast<void*>(&Descriptor::template detour<Fn>);
    }

    template <const auto& Fn>
    static void bindStatic() {
        raw_detour.store(staticDetour<Fn>(), std::memory_order_release);
    }

    static HOOKKIT_FORCEINLINE Ret viaDetour(Params... args) {
        if constexpr (Abi == Conv::eThiscall) {
            return Descriptor::invoke(Addr, std::forward<Params>(args)...);
        } else {
            return reinterpret_cast<RawFn>(Addr)(std::forward<Params>(args)...);
        }
    }

    HOOKKIT_FORCEINLINE Ret operator()(Params... args) const {
        if constexpr (Abi == Conv::eThiscall) {
            return callThiscall(std::forward<Params>(args)...);
        } else {
            detail::QuiesceGuard guard(in_flight, detaching, slots);
            return reinterpret_cast<RawFn>(target.load(std::memory_order_seq_cst))(std::forward<Params>(args)...);
        }
    }

private:
    template <typename This, typename... Rest>
    HOOKKIT_FORCEINLINE Ret callThiscall(This&& self, Rest&&... rest) const {
        detail::QuiesceGuard guard(in_flight, detaching, slots);
        return Descriptor::invoke(
            target.load(std::memory_order_seq_cst), std::forward<This>(self), std::forward<Rest>(rest)...);
    }
};

/**
 * @brief Defines a Hook struct and a wrapper function inside a class.
 * @param NAME The name of the function/hook.
 * @param ADDR The target memory address.
 * @param ABI The calling convention (e.g., hookkit::Conv::eCdecl).
 * @param RET The return type.
 * @param ... The parameter types.
 */
#define HOOKKIT_HOOK(NAME, ADDR, ABI, RET, ...)                                             \
    struct NAME##_tag {};                                                                   \
    struct NAME##_hook : ::hookkit::Hook<NAME##_tag, (ADDR), (ABI), RET, ##__VA_ARGS__> {}; \
    template <typename... Args>                                                             \
    HOOKKIT_FORCEINLINE RET NAME(Args&&... args) {                                          \
        if constexpr ((ABI) == ::hookkit::Conv::eThiscall) {                                \
            return NAME##_hook{}(this, std::forward<Args>(args)...);                        \
        } else {                                                                            \
            return NAME##_hook{}(std::forward<Args>(args)...);                              \
        }                                                                                   \
    }                                                                                       \
    static_assert(std::is_empty_v<NAME##_hook>, #NAME "_hook must stay zero-sized" HOOKKIT_ZERO_SIZED_WHY_)

/**
 * @brief Defines a zero-sized Hook struct handle (can be used globally).
 * @param NAME The name of the struct handle.
 * @param ADDR The target memory address.
 * @param ABI The calling convention.
 * @param RET The return type.
 * @param ... The parameter types.
 */
#define HOOKKIT_HOOK_HANDLE(NAME, ADDR, ABI, RET, ...)                               \
    struct NAME##_tag {};                                                            \
    struct NAME : ::hookkit::Hook<NAME##_tag, (ADDR), (ABI), RET, ##__VA_ARGS__> {}; \
    using NAME##_hook = NAME;                                                        \
    static_assert(std::is_empty_v<NAME>, #NAME " must stay zero-sized" HOOKKIT_ZERO_SIZED_WHY_)

struct NamedTarget {
    std::string_view name;
    std::uintptr_t addr;
};

/**
 * @brief Defines a zero-sized Hook struct whose branch targets are addressed
 *        by human-readable string names rather than raw indices.
 *
 *        Works at any patch site (call, jmp, inline hook, …) — the name refers
 *        to the *target labels*, not the kind of instruction being hooked.
 *
 * Usage:
 * @code
 *   HOOKKIT_NAMED_HOOK(func_hook, 0x...,
 *       {"ret",      0x...},
 *       {"fallback", 0x...},
 *       {"skip",     0x...});
 *
 *   // Retrieve a target at runtime:
 *   std::uintptr_t addr = func_hook::target("fallback");
 * @endcode
 *
 * @param NAME      The name of the generated struct.
 * @param ADDR      The address of the patch site to hook.
 * @param ...       Brace-initialised @c ::hookkit::NamedTarget entries,
 *                  e.g. @c {"ret", 0x...}.
 */
#define HOOKKIT_NAMED_HOOK(NAME, ADDR, ...)                                              \
    struct NAME##_tag {};                                                                \
    struct NAME : ::hookkit::Hook<NAME##_tag, (ADDR), ::hookkit::Conv::eCdecl, void> {   \
        static constexpr ::hookkit::NamedTarget kTargets[] = {__VA_ARGS__};              \
        static constexpr std::uintptr_t target(std::string_view name) {                  \
            for (const auto& t : kTargets) {                                             \
                if (t.name == name) { return t.addr; }                                   \
            }                                                                            \
            if (std::is_constant_evaluated()) {                                          \
                throw "hookkit: unknown target for " #NAME                               \
                      "::target(name), no label in its HOOKKIT_NAMED_HOOK list matches"; \
            }                                                                            \
            assert(false &&                                                              \
                "hookkit: unknown target for " #NAME                                     \
                "::target(name), no label in its HOOKKIT_NAMED_HOOK list matches");      \
            return 0;                                                                    \
        }                                                                                \
    };                                                                                   \
    using NAME##_hook = NAME;                                                            \
    static_assert(std::is_empty_v<NAME>, #NAME " must stay zero-sized" HOOKKIT_ZERO_SIZED_WHY_)

}  // namespace hookkit
