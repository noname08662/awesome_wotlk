#pragma once

#include <immintrin.h>

#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>

#include "abi_macros.h"

namespace hookkit {

enum class Conv { eCdecl, eStdcall, eFastcall, eVectorcall, eThiscall, eUsercall, eUserpurge };

template <Conv Abi, typename Ret, typename... Params>
struct Desc;

namespace detail {
inline constexpr std::size_t kQuiesceSlots = 16;

struct alignas(64) QuiesceSlot {
    std::atomic<std::uint32_t> n{0};
};

struct QuiesceSlots {
    QuiesceSlot slot[kQuiesceSlots];

    [[nodiscard]]
    bool idle(int skip = -1) const {
        for (std::size_t i = 0; i < kQuiesceSlots; ++i) {
            if (static_cast<int>(i) == skip) { continue; }
            if (slot[i].n.load(std::memory_order_seq_cst) != 0) { return false; }
        }
        return true;
    }
};

inline constinit std::atomic<std::uint32_t> quiesce_slot_owners{0};

inline thread_local int quiesce_thread_slot = -2;

struct QuiesceSlotOwner {
    int idx = -1;

    ~QuiesceSlotOwner() {
        const int mine = idx;
        quiesce_thread_slot = -1;
        if (mine >= 0) { quiesce_slot_owners.fetch_and(~(1u << mine), std::memory_order_acq_rel); }
    }
};

inline thread_local QuiesceSlotOwner quiesce_slot_owner;

__declspec(noinline) inline int claimQuiesceSlot() {
    std::uint32_t owners = quiesce_slot_owners.load(std::memory_order_relaxed);
    for (;;) {
        constexpr std::uint32_t kAll = (kQuiesceSlots >= 32) ? ~0u : ((1u << kQuiesceSlots) - 1u);
        const std::uint32_t free_bits = ~owners & kAll;
        if (free_bits == 0) {
            quiesce_thread_slot = -1;
            return -1;
        }
        const unsigned bit = static_cast<unsigned>(std::countr_zero(free_bits));
        if (quiesce_slot_owners.compare_exchange_weak(owners, owners | (1u << bit), std::memory_order_acq_rel)) {
            quiesce_slot_owner.idx = static_cast<int>(bit);
            quiesce_thread_slot = static_cast<int>(bit);
            return static_cast<int>(bit);
        }
    }
}

class QuiesceGuard {
public:
    HOOKKIT_FORCEINLINE QuiesceGuard(
        std::atomic<std::uint32_t>& in_flight, std::atomic<bool>& detaching, QuiesceSlots& slots) {
        const int idx = quiesce_thread_slot;
        if (idx >= 0) [[likely]] {
            std::atomic<std::uint32_t>& n = slots.slot[idx].n;
            n.store(n.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
            std::atomic_signal_fence(std::memory_order_seq_cst);
            if (!detaching.load(std::memory_order_relaxed)) [[likely]] {
                slot_ = &n;
                return;
            }
            n.store(n.load(std::memory_order_relaxed) - 1, std::memory_order_release);
        }
        enterSlow(in_flight, detaching, slots);
    }

    ~QuiesceGuard() {
        if (slot_ != nullptr) [[likely]] {
            slot_->store(slot_->load(std::memory_order_relaxed) - 1, std::memory_order_release);
        } else {
            in_flight_->fetch_sub(1, std::memory_order_seq_cst);
        }
    }

    QuiesceGuard(const QuiesceGuard&) = delete;
    QuiesceGuard& operator=(const QuiesceGuard&) = delete;
    QuiesceGuard(QuiesceGuard&&) = delete;
    QuiesceGuard& operator=(QuiesceGuard&&) = delete;

private:
    __declspec(noinline) void enterSlow(
        std::atomic<std::uint32_t>& in_flight, std::atomic<bool>& detaching, QuiesceSlots& slots) {
        int idx = quiesce_thread_slot;
        if (idx == -2) { idx = claimQuiesceSlot(); }

        if (idx >= 0) {
            std::atomic<std::uint32_t>& n = slots.slot[idx].n;
            for (;;) {
                while (detaching.load(std::memory_order_acquire)) {
                    _mm_pause();  // detach is quiescing this hook's trampoline
                }
                n.store(n.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
                std::atomic_signal_fence(std::memory_order_seq_cst);
                if (!detaching.load(std::memory_order_relaxed)) {
                    slot_ = &n;
                    return;
                }
                n.store(n.load(std::memory_order_relaxed) - 1, std::memory_order_release);
            }
        }

        in_flight_ = &in_flight;
        for (;;) {
            in_flight.fetch_add(1, std::memory_order_seq_cst);
            if (!detaching.load(std::memory_order_seq_cst)) { return; }
            in_flight.fetch_sub(1, std::memory_order_seq_cst);
            while (detaching.load(std::memory_order_acquire)) {
                _mm_pause();
            }
        }
    }

    std::atomic<std::uint32_t>* slot_ = nullptr;
    std::atomic<std::uint32_t>* in_flight_ = nullptr;
};

template <typename Ret, const auto& Fn, typename... Params>
inline constexpr bool kDetourInvocable =
    std::is_invocable_r_v<Ret, const std::remove_cvref_t<decltype(Fn)>&, Params...>;

}  // namespace detail

#define HOOKKIT_BIND_SIG_MSG_                                                                                        \
    "hookkit: the HOOKKIT_BIND'd lambda/function is not callable as Ret(Params...) of this hook (for eThiscall the " \
    "first parameter is the class pointer, then the remaining Params)."

// tail of the hook-declaring macros' is_empty static_assert; not #undef'd, it expands wherever those macros are used
#define HOOKKIT_ZERO_SIZED_WHY_ \
    ": all hook state is static, and HookTransaction::attach/detach only take empty handle types"

#define HOOKKIT_DEFINE_DESC_(ABI_TAG, KEYWORD)                                                                     \
    template <typename Ret, typename... Params>                                                                    \
    struct Desc<ABI_TAG, Ret, Params...> {                                                                         \
        using ClassType = void;                                                                                    \
        using Args = std::tuple<Params...>;                                                                        \
        using RawFn = Ret(KEYWORD*)(Params...);                                                                    \
        using HandlerFn = Ret(HOOKKIT_CDECL*)(Params...);                                                          \
        template <std::atomic<HandlerFn>& Handler, std::atomic<std::uintptr_t>& Target,                            \
            std::atomic<std::uint32_t>& InFlight, std::atomic<bool>& Detaching, detail::QuiesceSlots& Slots>       \
        static Ret KEYWORD bridge(Params... args) {                                                                \
            if (HandlerFn fn = Handler.load(std::memory_order_acquire)) [[likely]]                                 \
            {                                                                                                      \
                return fn(std::forward<Params>(args)...);                                                          \
            }                                                                                                      \
            detail::QuiesceGuard guard(InFlight, Detaching, Slots);                                                \
            return reinterpret_cast<RawFn>(Target.load(std::memory_order_seq_cst))(std::forward<Params>(args)...); \
        }                                                                                                          \
        template <const auto& Fn>                                                                                  \
        static Ret KEYWORD detour(Params... args) {                                                                \
            static_assert(detail::kDetourInvocable<Ret, Fn, Params...>, HOOKKIT_BIND_SIG_MSG_);                    \
            HOOKKIT_INLINE_CALL return Fn(std::forward<Params>(args)...);                                          \
        }                                                                                                          \
    }

HOOKKIT_DEFINE_DESC_(Conv::eCdecl, HOOKKIT_CDECL);
HOOKKIT_DEFINE_DESC_(Conv::eStdcall, HOOKKIT_STDCALL);
HOOKKIT_DEFINE_DESC_(Conv::eFastcall, HOOKKIT_FASTCALL);
HOOKKIT_DEFINE_DESC_(Conv::eVectorcall, HOOKKIT_VECTORCALL);
#undef HOOKKIT_DEFINE_DESC_

#define HOOKKIT_TYPE(...) __VA_ARGS__

namespace detail {
template <bool Sret, typename Ret, typename This, typename... Params>
struct ThiscallImpl;

template <typename Ret, typename This, typename... Params>
struct ThiscallImpl<false, Ret, This, Params...> {
    using RawFn = Ret(HOOKKIT_FASTCALL*)(This, void*, Params...);
    using HandlerFn = Ret(HOOKKIT_CDECL*)(This, Params...);

    template <std::atomic<HandlerFn>& Handler, std::atomic<std::uintptr_t>& Target,
        std::atomic<std::uint32_t>& InFlight, std::atomic<bool>& Detaching, detail::QuiesceSlots& Slots>
    static Ret HOOKKIT_FASTCALL bridge(This self, void*, Params... args) {
        if (HandlerFn fn = Handler.load(std::memory_order_acquire)) [[likely]]
        {
            return fn(self, std::forward<Params>(args)...);
        }
        detail::QuiesceGuard guard(InFlight, Detaching, Slots);
        return reinterpret_cast<RawFn>(Target.load(std::memory_order_seq_cst))(
            self, nullptr, std::forward<Params>(args)...);
    }

    template <const auto& Fn>
    static Ret HOOKKIT_FASTCALL detour(This self, void*, Params... args) {
        static_assert(detail::kDetourInvocable<Ret, Fn, This, Params...>, HOOKKIT_BIND_SIG_MSG_);
        HOOKKIT_INLINE_CALL return Fn(self, std::forward<Params>(args)...);
    }

    static Ret invoke(std::uintptr_t target, This self, Params... args) {
        return reinterpret_cast<RawFn>(target)(self, nullptr, std::forward<Params>(args)...);
    }
};

template <typename Ret, typename This, typename... Params>
struct ThiscallImpl<true, Ret, This, Params...> {
    using RawFn = Ret*(HOOKKIT_FASTCALL*)(This, void*, Ret*, Params...);
    using HandlerFn = Ret(HOOKKIT_CDECL*)(This, Params...);

    static_assert(std::is_move_constructible_v<Ret>,
        "hookkit: a struct returned from an eThiscall hook must be move-constructible "
        "(Hook::operator() hands the original's result back by value).");

    template <std::atomic<HandlerFn>& Handler, std::atomic<std::uintptr_t>& Target,
        std::atomic<std::uint32_t>& InFlight, std::atomic<bool>& Detaching, detail::QuiesceSlots& Slots>
    static Ret* HOOKKIT_FASTCALL bridge(This self, void*, Ret* out, Params... args) {
        if (HandlerFn fn = Handler.load(std::memory_order_acquire)) [[likely]]
        {
            ::new (static_cast<void*>(out)) Ret(fn(self, std::forward<Params>(args)...));
            return out;
        }
        detail::QuiesceGuard guard(InFlight, Detaching, Slots);
        return reinterpret_cast<RawFn>(Target.load(std::memory_order_seq_cst))(
            self, nullptr, out, std::forward<Params>(args)...);
    }

    template <const auto& Fn>
    static Ret* HOOKKIT_FASTCALL detour(This self, void*, Ret* out, Params... args) {
        static_assert(detail::kDetourInvocable<Ret, Fn, This, Params...>, HOOKKIT_BIND_SIG_MSG_);
        HOOKKIT_INLINE_CALL ::new (static_cast<void*>(out)) Ret(Fn(self, std::forward<Params>(args)...));
        return out;
    }

    static HOOKKIT_FORCEINLINE Ret invoke(std::uintptr_t target, This self, Params... args) {
        alignas(Ret) unsigned char storage[sizeof(Ret)];
        Ret* const out = reinterpret_cast<Ret*>(storage);
        reinterpret_cast<RawFn>(target)(self, nullptr, out, std::forward<Params>(args)...);

        struct Destroy {
            Ret* p;

            ~Destroy() { p->~Ret(); }
        } destroy{out};

        return Ret(std::move(*out));
    }
};

}  // namespace detail

template <typename Ret, typename This, typename... Params>
struct Desc<Conv::eThiscall, Ret, This, Params...>
    : detail::ThiscallImpl<std::is_class_v<Ret> || std::is_union_v<Ret>, Ret, This, Params...> {
    using ClassType = This;
    using Args = std::tuple<Params...>;

    static_assert(std::is_void_v<Ret> || std::is_scalar_v<Ret> || std::is_class_v<Ret> || std::is_union_v<Ret>,
        "hookkit: eThiscall return type must be void, a scalar (integral/pointer/enum/floating-point) "
        "or a class/union (returned through MSVC's hidden pointer); references and arrays are not "
        "supported.");
};

}  // namespace hookkit
