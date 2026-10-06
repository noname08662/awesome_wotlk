#pragma once

#include <asmjit/core.h>
#include <asmjit/x86.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include <utility>

#include "abi_macros.h"
#include "accessor.h"
#include "desc.h"

namespace hookkit {

enum class Reg { eAx, eCx, eDx, eBx, eSp, eBp, eSi, eDi };

struct ArgLoc {
    bool is_reg = false;
    Reg reg = Reg::eAx;
    std::int32_t stack_offset = 0;

    static constexpr ArgLoc inReg(Reg r) { return ArgLoc{true, r, 0}; }

    static constexpr ArgLoc onStack(std::int32_t byte_offset) { return ArgLoc{false, Reg::eAx, byte_offset}; }
};

template <std::size_t N>
struct WildAbi {
    std::array<ArgLoc, N> args{};

    [[nodiscard]]
    constexpr std::size_t stackBytes() const {
        std::size_t max_end = 0;
        for (const auto& a : args) {
            if (!a.is_reg) {
                std::size_t end = static_cast<std::size_t>(a.stack_offset) + 4;
                max_end = (std::max)(end, max_end);
            }
        }
        return max_end;
    }
};

namespace detail {
inline constinit utils::Accessor<asmjit::JitRuntime, struct WildAbiJitRuntimeTag> wild_abi_jit_runtime{};

class JitErrorHandler : public asmjit::ErrorHandler {
public:
    void handle_error(asmjit::Error err, const char* message, asmjit::BaseEmitter*) override {
        had_error_ = true;
        (void)err;
        (void)message;
    }

    [[nodiscard]]
    bool hadError() const {
        return had_error_;
    }

private:
    bool had_error_ = false;
};

// sizeof(void) is ill-formed even when short-circuited in a plain `||`, so void has to be split off first
template <typename Ret>
constexpr bool wildRetSupported() {
    if constexpr (std::is_void_v<Ret>) {
        return true;
    } else {
        return std::is_scalar_v<Ret> && sizeof(Ret) <= 4;
    }
}

inline asmjit::x86::Gp wildAbiMapReg(Reg r) {
    using namespace asmjit::x86;
    switch (r) {
        case Reg::eAx:
            return eax;
        case Reg::eCx:
            return ecx;
        case Reg::eDx:
            return edx;
        case Reg::eBx:
            return ebx;
        case Reg::eSp:
            return esp;
        case Reg::eBp:
            return ebp;
        case Reg::eSi:
            return esi;
        case Reg::eDi:
            return edi;
    }
    return eax;
}
}  // namespace detail

#define HOOKKIT_NO_ESP_REG_ARG_MSG_                                                                \
    "WildAbi: Reg::eSp is only meaningful for the incoming-call endpoint (buildEndpoint) and has " \
    "no well-defined value when hookkit initiates the call itself (operator()/buildInvoker). "     \
    "Remove Reg::eSp from this AbiSpec, or bind the endpoint via HOOKKIT_BIND_RAW and avoid "      \
    "calling through operator() for this hook."

template <typename Tag, std::uintptr_t Addr, Conv Abi, typename Ret, auto AbiSpec, typename... Params>
struct WildHook {
    static_assert(Abi == Conv::eUsercall || Abi == Conv::eUserpurge,
        "WildHook is only for Conv::eUsercall / Conv::eUserpurge; use Hook "
        "(HOOKKIT_HOOK) for cdecl/stdcall/fastcall/thiscall.");
    static_assert(std::is_same_v<std::decay_t<decltype(AbiSpec)>, WildAbi<sizeof...(Params)>>,
        "AbiSpec must be a WildAbi<N> with N == the number of Params (one ArgLoc per parameter, in order)");
    static_assert(((sizeof(Params) <= 4) && ...),
        "WildAbi bridging currently supports only <=4-byte (register-width) "
        "parameters on x86; 64-bit params (std::int64_t/double/...) aren't handled.");
    static_assert(detail::wildRetSupported<Ret>(),
        "WildHook: Ret must be void, or a scalar type (integral, pointer, "
        "enum, or floating-point) of at most 4 bytes. Struct/union return "
        "types are rejected even when <=4 bytes: some ABIs return non-scalar "
        "types via a hidden output pointer, which is an extra implicit "
        "argument that WildAbi/Params has no way to represent -- accepting "
        "one here would silently corrupt argument marshaling, not just the "
        "return value.");

    using RawFn = Ret(HOOKKIT_CDECL*)(Params...);
    using HandlerFn = RawFn;
    using ReturnType = Ret;
    using Args = std::tuple<Params...>;

    static constexpr Conv kAbi = Abi;
    static constexpr std::uintptr_t kAddress = Addr;
    static constexpr std::size_t kArity = sizeof...(Params);

    inline static constinit std::atomic<std::uintptr_t> target{Addr};
    inline static std::atomic<HandlerFn> hook{nullptr};
    inline static std::atomic<bool> attached{false};

    inline static std::atomic<std::uint32_t> in_flight{0};
    inline static std::atomic<bool> detaching{false};
    inline static detail::QuiesceSlots slots{};

    static_assert(
        sizeof(std::atomic<std::uintptr_t>) == sizeof(void*) && alignof(std::atomic<std::uintptr_t>) == alignof(void*),
        "hookkit: std::atomic<uintptr_t> must be layout-compatible with void* -- patchSlot() reads/"
        "writes it as a raw PVOID* for both the MinHook in/out idiom and the bridge's own load");

    HOOKKIT_FORCEINLINE Ret operator()(Params... args) const {
        static_assert(validateNoEspRegArg(), HOOKKIT_NO_ESP_REG_ARG_MSG_);
        RawFn fn = cached_invoker.load(std::memory_order_acquire);
        if (fn == nullptr) [[unlikely]] {
            fn = getOrCreateInvoker();
            if (fn) { cached_invoker.store(fn, std::memory_order_release); }
        }
        detail::QuiesceGuard guard(in_flight, detaching, slots);
        return fn(std::forward<Params>(args)...);
    }

    static Ret HOOKKIT_CDECL bridgeImpl(Params... args) {
        if (HandlerFn fn = hook.load(std::memory_order_acquire)) { return fn(std::forward<Params>(args)...); }
        detail::QuiesceGuard guard(in_flight, detaching, slots);
        return cached_invoker.load(std::memory_order_acquire)(std::forward<Params>(args)...);
    }

    static constexpr auto kBridge = &bridgeImpl;

    inline static std::atomic<void*> direct_handler{nullptr};

    template <const auto& Fn>
    static Ret HOOKKIT_CDECL directThunk(Params... args) {
        static_assert(detail::kDetourInvocable<Ret, Fn, Params...>, HOOKKIT_BIND_SIG_MSG_);
        HOOKKIT_INLINE_CALL return Fn(std::forward<Params>(args)...);
    }

    template <const auto& Fn>
    static void bindStatic() {
        direct_handler.store(reinterpret_cast<void*>(&directThunk<Fn>), std::memory_order_release);
    }

    static RawFn getOrCreateInvoker() {
        static RawFn built = buildInvoker();
        return built;
    }

    static void* getOrCreateEndpoint() {
        static void* built = buildEndpoint();
        return built;
    }

    inline static std::atomic<void*> raw_detour{nullptr};

    static void* resolveDetour() {
        if (void* ptr = raw_detour.load(std::memory_order_acquire)) { return ptr; }
        if (RawFn invoker = getOrCreateInvoker()) { cached_invoker.store(invoker, std::memory_order_release); }
        return getOrCreateEndpoint();
    }

    static void** patchSlot() { return reinterpret_cast<void**>(&target); }

    static bool quiescent(bool exclude_caller = false) {
        if (in_flight.load(std::memory_order_seq_cst) != 0) { return false; }
        return slots.idle(exclude_caller ? detail::quiesce_thread_slot : -1);
    }

private:
    static constexpr bool calleeCleans() { return Abi == Conv::eUserpurge; }

    static constexpr bool validateNoDuplicateRegs() {
        for (std::size_t i = 0; i < sizeof...(Params); ++i) {
            if (!AbiSpec.args[i].is_reg) { continue; }
            for (std::size_t j = i + 1; j < sizeof...(Params); ++j) {
                if (AbiSpec.args[j].is_reg && AbiSpec.args[j].reg == AbiSpec.args[i].reg) { return false; }
            }
        }
        return true;
    }

    static_assert(validateNoDuplicateRegs(),
        "WildAbi: duplicate register assignment across parameters -- two ArgLoc::inReg() entries name the same Reg");

    static constexpr bool validateNoDuplicateStackOffsets() {
        for (std::size_t i = 0; i < sizeof...(Params); ++i) {
            if (AbiSpec.args[i].is_reg) { continue; }
            for (std::size_t j = i + 1; j < sizeof...(Params); ++j) {
                if (!AbiSpec.args[j].is_reg && AbiSpec.args[j].stack_offset == AbiSpec.args[i].stack_offset) {
                    return false;
                }
            }
        }
        return true;
    }

    static_assert(validateNoDuplicateStackOffsets(),
        "WildAbi: duplicate stack_offset across parameters -- two ArgLoc::onStack() entries share a byte offset");

    static constexpr bool validateStackOffsets() {
        for (std::size_t i = 0; i < sizeof...(Params); ++i) {
            const auto& a = AbiSpec.args[i];
            if (!a.is_reg && (a.stack_offset < 0 || (a.stack_offset % 4) != 0)) { return false; }
        }
        return true;
    }

    static_assert(validateStackOffsets(),
        "WildAbi: stack_offset must be a non-negative multiple of 4 (bytes past the return address, so 0 is [esp+4] "
        "at entry)");

    static constexpr bool validateNoEspRegArg() {
        for (std::size_t i = 0; i < sizeof...(Params); ++i) {
            if (AbiSpec.args[i].is_reg && AbiSpec.args[i].reg == Reg::eSp) { return false; }
        }
        return true;
    }

    static RawFn buildInvoker() {
        using namespace asmjit;
        constexpr std::size_t kN = sizeof...(Params);
        const std::size_t stack_bytes = AbiSpec.stackBytes();

        constexpr std::array kSaveReg = {true, true, true};
        constexpr std::size_t kSavedCount = 3;

        CodeHolder code;
        if (const Error init_err = code.init(detail::wild_abi_jit_runtime->environment()); init_err != kErrorOk) {
            assert(init_err == kErrorOk &&
                "hookkit: WildHook::buildInvoker: CodeHolder::init failed, init_err is the asmjit::Error");
            return nullptr;
        }
        detail::JitErrorHandler err_handler;
        code.set_error_handler(&err_handler);
        x86::Assembler a(&code);

        // prologue
        a.push(x86::ebp);
        if (kSaveReg[0]) { a.push(x86::esi); }
        if (kSaveReg[1]) { a.push(x86::edi); }
        if (kSaveReg[2]) { a.push(x86::ebx); }

        if (stack_bytes != 0u) { a.sub(x86::esp, static_cast<std::int32_t>(stack_bytes)); }

        const auto arg_offset = [&](std::size_t i) -> std::int32_t {
            return static_cast<std::int32_t>(stack_bytes + (2 + kSavedCount) * 4 + 4 * i);
        };

        for (std::size_t i = 0; i < kN; ++i) {
            const ArgLoc& loc = AbiSpec.args[i];
            if (!loc.is_reg) {
                a.mov(x86::eax, x86::dword_ptr(x86::esp, arg_offset(i)));
                a.mov(x86::dword_ptr(x86::esp, loc.stack_offset), x86::eax);
            }
        }

        for (std::size_t i = 0; i < kN; ++i) {
            const ArgLoc& loc = AbiSpec.args[i];
            if (loc.is_reg) { a.mov(detail::wildAbiMapReg(loc.reg), x86::dword_ptr(x86::esp, arg_offset(i))); }
        }

        a.call(x86::dword_ptr(reinterpret_cast<std::uint64_t>(&target)));

        // epilogue
        if (!calleeCleans() && (stack_bytes != 0u)) { a.add(x86::esp, static_cast<std::int32_t>(stack_bytes)); }
        if (kSaveReg[2]) { a.pop(x86::ebx); }
        if (kSaveReg[1]) { a.pop(x86::edi); }
        if (kSaveReg[0]) { a.pop(x86::esi); }
        a.pop(x86::ebp);
        a.ret();

        if (err_handler.hadError()) {
            assert(!err_handler.hadError() &&
                "hookkit: WildHook::buildInvoker: asmjit rejected an emit, break in "
                "detail::JitErrorHandler::handle_error for its error and message");
            return nullptr;
        }
        RawFn fn = nullptr;
        if (const Error add_err = detail::wild_abi_jit_runtime->add(&fn, &code); add_err != kErrorOk) {
            assert(add_err == kErrorOk &&
                "hookkit: WildHook::buildInvoker: JitRuntime::add failed, add_err is the asmjit::Error");
            return nullptr;
        }
        return fn;
    }

    static void* buildEndpoint() {
        using namespace asmjit;
        constexpr std::size_t kN = sizeof...(Params);
        const std::size_t stack_bytes = AbiSpec.stackBytes();

        CodeHolder code;
        if (const Error init_err = code.init(detail::wild_abi_jit_runtime->environment()); init_err != kErrorOk) {
            assert(init_err == kErrorOk &&
                "hookkit: WildHook::buildEndpoint: CodeHolder::init failed, init_err is the asmjit::Error");
            return nullptr;
        }
        detail::JitErrorHandler err_handler;
        code.set_error_handler(&err_handler);
        x86::Assembler a(&code);

        constexpr bool kSaveEax = std::is_void_v<Ret>;
        std::size_t bytes_pushed = 0;
        if constexpr (kSaveEax) {
            a.push(x86::eax);
            bytes_pushed += 4;
        }
        a.push(x86::ecx);
        a.push(x86::edx);
        bytes_pushed += 8;

        for (std::size_t idx = kN; idx-- > 0;) {
            const ArgLoc& loc = AbiSpec.args[idx];
            if (loc.is_reg) {
                if (loc.reg == Reg::eSp) {
                    a.push(x86::esp);
                    a.add(x86::dword_ptr(x86::esp, 0), static_cast<std::int32_t>(bytes_pushed + 4));
                } else {
                    a.push(detail::wildAbiMapReg(loc.reg));
                }
            } else {
                a.push(x86::dword_ptr(x86::esp, static_cast<std::int32_t>(bytes_pushed + 4 + loc.stack_offset)));
            }
            bytes_pushed += 4;
        }

        void* const direct = direct_handler.load(std::memory_order_acquire);
        a.mov(x86::eax,
            asmjit::imm(direct ? reinterpret_cast<std::uintptr_t>(direct) : reinterpret_cast<std::uintptr_t>(kBridge)));
        a.call(x86::eax);

        if (kN > 0) { a.add(x86::esp, static_cast<std::int32_t>(kN * 4)); }

        a.pop(x86::edx);
        a.pop(x86::ecx);
        if constexpr (kSaveEax) { a.pop(x86::eax); }

        if (calleeCleans()) {
            a.ret(static_cast<std::int32_t>(stack_bytes));
        } else {
            a.ret();
        }

        if (err_handler.hadError()) {
            assert(!err_handler.hadError() &&
                "hookkit: WildHook::buildEndpoint: asmjit rejected an emit, break in "
                "detail::JitErrorHandler::handle_error for its error and message");
            return nullptr;
        }
        void* fn = nullptr;
        if (const Error add_err = detail::wild_abi_jit_runtime->add(&fn, &code); add_err != kErrorOk) {
            assert(add_err == kErrorOk &&
                "hookkit: WildHook::buildEndpoint: JitRuntime::add failed, add_err is the asmjit::Error");
            return nullptr;
        }
        return fn;
    }

public:
    inline static std::atomic<RawFn> cached_invoker{nullptr};
};

#undef HOOKKIT_NO_ESP_REG_ARG_MSG_

/**
 * @brief Defines a WildHook struct and a wrapper function inside a class.
 *        Used for custom calling conventions (eUsercall/eUserpurge).
 * @param NAME The name of the function/hook.
 * @param ADDR The target memory address.
 * @param ABI The calling convention (e.g., hookkit::Conv::eUsercall).
 * @param RET The return type.
 * @param ABI_SPEC The hookkit::WildAbi spec detailing register/stack locations.
 * @param ... The parameter types.
 */
#define HOOKKIT_HOOK_WILD(NAME, ADDR, ABI, RET, ABI_SPEC, ...)                                              \
    struct NAME##_tag {};                                                                                   \
    struct NAME##_hook : ::hookkit::WildHook<NAME##_tag, (ADDR), (ABI), RET, (ABI_SPEC), ##__VA_ARGS__> {}; \
    template <typename... Args>                                                                             \
    HOOKKIT_FORCEINLINE RET NAME(Args&&... args) {                                                          \
        return NAME##_hook{}(std::forward<Args>(args)...);                                                  \
    }                                                                                                       \
    static_assert(std::is_empty_v<NAME##_hook>, #NAME "_hook must stay zero-sized" HOOKKIT_ZERO_SIZED_WHY_)

/**
 * @brief Defines a zero-sized WildHook struct handle (can be used globally).
 *        Used for custom calling conventions (eUsercall/eUserpurge).
 * @param NAME The name of the struct handle.
 * @param ADDR The target memory address.
 * @param ABI The calling convention.
 * @param RET The return type.
 * @param ABI_SPEC The hookkit::WildAbi spec detailing register/stack locations.
 * @param ... The parameter types.
 */
#define HOOKKIT_HOOK_WILD_HANDLE(NAME, ADDR, ABI, RET, ABI_SPEC, ...)                                \
    struct NAME##_tag {};                                                                            \
    struct NAME : ::hookkit::WildHook<NAME##_tag, (ADDR), (ABI), RET, (ABI_SPEC), ##__VA_ARGS__> {}; \
    using NAME##_hook = NAME;                                                                        \
    static_assert(std::is_empty_v<NAME>, #NAME " must stay zero-sized" HOOKKIT_ZERO_SIZED_WHY_)

#define HOOKKIT_BIND_IMPL2_(Line) _hook_bind_var_##Line
#define HOOKKIT_BIND_IMPL_(Line) HOOKKIT_BIND_IMPL2_(Line)
#define HOOKKIT_BIND_LAM2_(Line) _hook_bind_lam_##Line
#define HOOKKIT_BIND_LAM_(Line) HOOKKIT_BIND_LAM2_(Line)
#define HOOKKIT_BIND_N_(ID, HOOK_TYPE, ...)                             \
    inline static constexpr auto HOOKKIT_BIND_LAM_(ID) = (__VA_ARGS__); \
    [[maybe_unused]]                                                    \
    inline static const auto HOOKKIT_BIND_IMPL_(ID) = (HOOK_TYPE::bindStatic<HOOKKIT_BIND_LAM_(ID)>(), 0)

/**
 * @brief Binds a lambda or function directly to a hook at the global/namespace scope.
 *        Avoids the need to create dummy variables or polluting initialize() functions.
 * @param HOOK_TYPE The type of the hook (e.g., sys::clipboardGetStr)
 * @param ... The lambda or function pointer to assign to the hook
 * @note __COUNTER__ is per-translation-unit, so uniqueness is only guaranteed
 *       when this is invoked directly in exactly one .cpp file. Invoking it
 *       from inside a header included by multiple .cpp files will mint a
 *       distinct symbol (and therefore run the assignment again) in each TU,
 *       rather than binding once as intended.
 *
 * @note Static bind: the handler is compiled into a target-ABI detour (no bridge thunk); it cannot be
 *       swapped/cleared at runtime. For a runtime-assignable handler assign `HOOK_TYPE::hook = ...` instead.
 *       The argument must be a captureless lambda or function pointer (a constant expression).
 */
#define HOOKKIT_BIND(HOOK_TYPE, ...) HOOKKIT_BIND_N_(__COUNTER__, HOOK_TYPE, __VA_ARGS__)

#define HOOKKIT_BIND_RAW_IMPL2_(Line) _hook_bind_raw_var_##Line
#define HOOKKIT_BIND_RAW_IMPL_(Line) HOOKKIT_BIND_RAW_IMPL2_(Line)
#define HOOKKIT_BIND_RAW(HOOK_TYPE, DETOUR_FUNC)                   \
    using namespace ::hookkit;                                     \
    [[maybe_unused]]                                               \
    inline static const auto HOOKKIT_BIND_RAW_IMPL_(__COUNTER__) = \
        (HOOK_TYPE::raw_detour = reinterpret_cast<void*>(DETOUR_FUNC), 0)

namespace detail {
template <class Hook>
struct RawBinder {
    template <class Make>
    int operator+(Make&& make) const {
        Hook::raw_detour = reinterpret_cast<void*>(make());
        return 0;
    }
};
}  // namespace detail

/**
 * @brief HOOKKIT_NAMED_HOOK + HOOKKIT_BIND_RAW in one: declares the named hook and binds the detour that the
 *        trailing captureless lambda body returns (e.g. a CallsiteTrampolineBuilder::build result).
 *        Inside the body, `NAME::target("label")` is available. Also declares `NAME##_hook`.
 *
 * @code
 *   HOOKKIT_NAMED_BIND_RAW(site, 0x..., {"jmpback", 0x...,}) {
 *       return CallsiteTrampolineBuilder{}.build(jmpTo(site::target("jmpback")));
 *   };
 * @endcode
 * @note Same one-TU rule as HOOKKIT_BIND_RAW. The trailing `;` is required.
 */
#define HOOKKIT_NAMED_BIND_RAW(NAME, ADDR, ...)  \
    using namespace ::hookkit;                   \
    HOOKKIT_NAMED_HOOK(NAME, ADDR, __VA_ARGS__); \
    [[maybe_unused]]                             \
    inline static const auto HOOKKIT_BIND_RAW_IMPL_(__COUNTER__) = ::hookkit::detail::RawBinder<NAME>{} + []()

namespace detail {
template <class Lambda>
inline constexpr Lambda kStaticLambda{};

template <class Hook>
struct StaticBinder {
    template <class Lambda>
    int operator+(Lambda) const {
        static_assert(std::is_empty_v<Lambda> && std::is_default_constructible_v<Lambda>,
            "HOOKKIT_HOOK_BIND: the handler must be a captureless lambda");
        Hook::template bindStatic<kStaticLambda<Lambda>>();
        return 0;
    }
};
}  // namespace detail

/**
 * @brief HOOKKIT_HOOK_HANDLE + HOOKKIT_BIND in one: declares the hook handle and statically binds the trailing
 *        captureless lambda as its handler. Also declares `NAME##_hook`.
 *
 * @code
 *   HOOKKIT_HOOK_BIND(site, 0x..., hookkit::Conv::eCdecl, int, int)[](int x) -> int {
 *       return site{}(x) + 1;
 *   };
 * @endcode
 * @note Same one-TU rule as HOOKKIT_BIND. The trailing `;` is required.
 */
#define HOOKKIT_HOOK_BIND(NAME, ADDR, ABI, RET, ...)          \
    HOOKKIT_HOOK_HANDLE(NAME, ADDR, ABI, RET, ##__VA_ARGS__); \
    [[maybe_unused]]                                          \
    inline static const auto HOOKKIT_BIND_IMPL_(__COUNTER__) = ::hookkit::detail::StaticBinder<NAME>{} +

#define HOOKKIT_DECL_NAKED_IMPL2_(HOOK_TYPE, ID)           \
    static void _hookkit_decl_naked_##ID();                \
    HOOKKIT_BIND_RAW(HOOK_TYPE, _hookkit_decl_naked_##ID); \
    static void __declspec(naked) _hookkit_decl_naked_##ID()

#define HOOKKIT_DECL_NAKED_IMPL_(HOOK_TYPE, ID) HOOKKIT_DECL_NAKED_IMPL2_(HOOK_TYPE, ID)

/**
 * @brief Defines a naked detour and binds it to the specified hook type automatically.
 * @param HOOK_TYPE The type of the hook to bind to.
 */
#define HOOKKIT_DECL_NAKED(HOOK_TYPE) HOOKKIT_DECL_NAKED_IMPL_(HOOK_TYPE, __COUNTER__)

}  // namespace hookkit
