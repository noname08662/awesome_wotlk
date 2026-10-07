#pragma once

#include <asmjit/core.h>
#include <asmjit/x86.h>

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "hook.h"
#include "wildabi.h"

namespace hookkit {

namespace detail {
inline void hookkitDefaultConventionProbe() {}

static_assert(std::is_same_v<decltype(&hookkitDefaultConventionProbe), void(HOOKKIT_CDECL*)()>,
    "hookkit: this TU's default calling convention is no longer __cdecl -- "
    "HOOKKIT_LAMBDA_ADDR (trampoline.h) needs updating before it's safe to use again");
}  // namespace detail

#define HOOKKIT_LAMBDA_ADDR(...) (reinterpret_cast<std::uintptr_t>(+(__VA_ARGS__)))

class TrampolineStepHandle {
public:
    template <typename Fn>
    TrampolineStepHandle(Fn&& fn, std::int32_t push_delta) : emit_(std::forward<Fn>(fn)), push_delta_(push_delta) {}

    ~TrampolineStepHandle() = default;

    TrampolineStepHandle(TrampolineStepHandle&&) noexcept = default;
    TrampolineStepHandle& operator=(TrampolineStepHandle&&) noexcept = default;
    TrampolineStepHandle(const TrampolineStepHandle&) = delete;
    TrampolineStepHandle& operator=(const TrampolineStepHandle&) = delete;

    TrampolineStepHandle&& withScratch(Reg r) && {
        assert(r != Reg::eSp &&
            "hookkit: TrampolineStepHandle::withScratch: esp can't be used as a scratch register, the step would "
            "overwrite the stack pointer");
        scratch_override_ = r;
        return std::move(*this);
    }

    void emit(asmjit::x86::Assembler& a, const asmjit::x86::Gp& default_scratch) const {
        emit_(a, scratch_override_ ? detail::wildAbiMapReg(*scratch_override_) : default_scratch);
    }

    [[nodiscard]]
    std::int32_t pushDelta() const {
        return push_delta_;
    }

private:
    std::function<void(asmjit::x86::Assembler&, const asmjit::x86::Gp&)> emit_;
    std::int32_t push_delta_;
    std::optional<Reg> scratch_override_;
};

// lea <scratch>, [base + disp]; push <scratch>
inline TrampolineStepHandle pushLea(Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp& scratch) {
        a.lea(scratch, asmjit::x86::ptr(detail::wildAbiMapReg(base), disp));
        a.push(scratch);
    }, 4};
}

// push reg
inline TrampolineStepHandle pushReg(Reg reg) {
    return TrampolineStepHandle{
        [reg](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.push(detail::wildAbiMapReg(reg)); }, 4};
}

// pop reg
inline TrampolineStepHandle popReg(Reg reg) {
    return TrampolineStepHandle{
        [reg](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.pop(detail::wildAbiMapReg(reg)); }, 0};
}

// push reg... , in the given order
template <typename... Regs>
    requires(sizeof...(Regs) > 0) && (std::is_same_v<Regs, Reg> && ...) TrampolineStepHandle pushRegs(Regs... regs) {
    return TrampolineStepHandle{[regs...](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        (a.push(detail::wildAbiMapReg(regs)), ...);
    }, static_cast<std::int32_t>(4 * sizeof...(Regs))};
}

// pop reg... , undoing a pushRegs()
template <typename... Regs>
    requires(sizeof...(Regs) > 0) && (std::is_same_v<Regs, Reg> && ...) TrampolineStepHandle popRegs(Regs... regs) {
    const std::array<Reg, sizeof...(Regs)> ordered{regs...};
    return TrampolineStepHandle{[ordered](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        for (auto it = ordered.rbegin(); it != ordered.rend(); ++it) {
            a.pop(detail::wildAbiMapReg(*it));
        }
    }, -static_cast<std::int32_t>(4 * ordered.size())};
}

// push imm32
inline TrampolineStepHandle pushImm(std::int32_t value) {
    const auto uvalue = static_cast<std::uint32_t>(value);
    return TrampolineStepHandle{
        [uvalue](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.push(asmjit::imm(uvalue)); }, 4};
}

// push dword ptr [base + disp]
inline TrampolineStepHandle pushMem(Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.push(asmjit::x86::dword_ptr(detail::wildAbiMapReg(base), disp));
    }, 4};
}

// push dword ptr [addr]  (fixed absolute address, no base register)
inline TrampolineStepHandle pushAbs(std::uintptr_t addr) {
    return TrampolineStepHandle{[addr](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.push(asmjit::x86::dword_ptr(static_cast<std::uint64_t>(addr)));
    }, 4};
}

// add reg, imm32
inline TrampolineStepHandle addReg(Reg reg, std::int32_t value) {
    return TrampolineStepHandle{[reg, value](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.add(detail::wildAbiMapReg(reg), asmjit::imm(value));
    }, 0};
}

// sub reg, imm32
inline TrampolineStepHandle subReg(Reg reg, std::int32_t value) {
    return TrampolineStepHandle{[reg, value](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.sub(detail::wildAbiMapReg(reg), asmjit::imm(value));
    }, 0};
}

// and reg, imm32
inline TrampolineStepHandle andReg(Reg reg, std::int32_t mask) {
    return TrampolineStepHandle{[reg, mask](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.and_(detail::wildAbiMapReg(reg), asmjit::imm(mask));
    }, 0};
}

// or reg, imm32
inline TrampolineStepHandle orReg(Reg reg, std::int32_t mask) {
    return TrampolineStepHandle{[reg, mask](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.or_(detail::wildAbiMapReg(reg), asmjit::imm(mask));
    }, 0};
}

// xor reg, imm32
inline TrampolineStepHandle xorReg(Reg reg, std::int32_t mask) {
    return TrampolineStepHandle{[reg, mask](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.xor_(detail::wildAbiMapReg(reg), asmjit::imm(mask));
    }, 0};
}

// mov reg, imm32
inline TrampolineStepHandle movRegImm(Reg reg, std::uint32_t value) {
    return TrampolineStepHandle{[reg, value](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.mov(detail::wildAbiMapReg(reg), asmjit::imm(value));
    }, 0};
}

// mov reg, dword ptr [base + disp]
inline TrampolineStepHandle movRegMem(Reg reg, Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[reg, base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.mov(detail::wildAbiMapReg(reg), asmjit::x86::dword_ptr(detail::wildAbiMapReg(base), disp));
    }, 0};
}

// mov reg, dword ptr [addr]  (fixed absolute address, no base register)
inline TrampolineStepHandle movRegAbs(Reg reg, std::uintptr_t addr) {
    return TrampolineStepHandle{[reg, addr](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.mov(detail::wildAbiMapReg(reg), asmjit::x86::dword_ptr(static_cast<std::uint64_t>(addr)));
    }, 0};
}

// mov dst, src
inline TrampolineStepHandle movRegReg(Reg dst, Reg src) {
    return TrampolineStepHandle{[dst, src](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.mov(detail::wildAbiMapReg(dst), detail::wildAbiMapReg(src));
    }, 0};
}

// mov dword ptr [base + disp], reg
inline TrampolineStepHandle movMemReg(Reg base, std::int32_t disp, Reg src) {
    return TrampolineStepHandle{[base, disp, src](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.mov(asmjit::x86::dword_ptr(detail::wildAbiMapReg(base), disp), detail::wildAbiMapReg(src));
    }, 0};
}

// mov dword ptr [base + disp], imm32
inline TrampolineStepHandle movMemImm(Reg base, std::int32_t disp, std::uint32_t value) {
    return TrampolineStepHandle{[base, disp, value](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.mov(asmjit::x86::dword_ptr(detail::wildAbiMapReg(base), disp), asmjit::imm(value));
    }, 0};
}

// mov dword ptr [addr], reg  (fixed absolute address, no base register)
inline TrampolineStepHandle movAbsReg(std::uintptr_t addr, Reg src) {
    return TrampolineStepHandle{[addr, src](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.mov(asmjit::x86::dword_ptr(static_cast<std::uint64_t>(addr)), detail::wildAbiMapReg(src));
    }, 0};
}

// lea reg, [base + disp]
inline TrampolineStepHandle leaReg(Reg reg, Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[reg, base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.lea(detail::wildAbiMapReg(reg), asmjit::x86::ptr(detail::wildAbiMapReg(base), disp));
    }, 0};
}

// cmp dword ptr [base + disp], imm32
inline TrampolineStepHandle cmpMemImm(Reg base, std::int32_t disp, std::int32_t value) {
    return TrampolineStepHandle{[base, disp, value](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.cmp(asmjit::x86::dword_ptr(detail::wildAbiMapReg(base), disp), asmjit::imm(value));
    }, 0};
}

// cmp byte ptr [base + disp], <low byte of src>
inline TrampolineStepHandle cmpMemReg8(Reg base, std::int32_t disp, Reg src) {
    assert((src == Reg::eAx || src == Reg::eCx || src == Reg::eDx || src == Reg::eBx) &&
        "hookkit: cmpMemReg8: only eax/ecx/edx/ebx have an addressable low byte in 32-bit mode -- "
        "sil/dil/spl/bpl need a REX prefix, which doesn't exist on x86");
    return TrampolineStepHandle{[base, disp, src](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.cmp(asmjit::x86::byte_ptr(detail::wildAbiMapReg(base), disp), detail::wildAbiMapReg(src).r8_lo());
    }, 0};
}

// push all general-purpose registers
inline TrampolineStepHandle pushadStep() {
    return TrampolineStepHandle{[](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.pushad(); }, 0};
}

// pop all general-purpose registers
inline TrampolineStepHandle popadStep() {
    return TrampolineStepHandle{[](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.popad(); }, 0};
}

// push EFLAGS
inline TrampolineStepHandle pushfdStep() {
    return TrampolineStepHandle{[](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.pushfd(); }, 0};
}

// pop EFLAGS
inline TrampolineStepHandle popfdStep() {
    return TrampolineStepHandle{[](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.popfd(); }, 0};
}

// push all general-purpose registers, then EFLAGS
inline TrampolineStepHandle pushAllStep() {
    return TrampolineStepHandle{[](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.pushad();
        a.pushfd();
    }, 0};
}

// pop EFLAGS, then all general-purpose registers -- undoes pushAllStep()
inline TrampolineStepHandle popAllStep() {
    return TrampolineStepHandle{[](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.popfd();
        a.popad();
    }, 0};
}

// fld1
inline TrampolineStepHandle fld1Step() {
    return TrampolineStepHandle{[](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.fld1(); }, 0};
}

// fld dword ptr [base + disp]
inline TrampolineStepHandle fldMem(Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.fld(asmjit::x86::dword_ptr(detail::wildAbiMapReg(base), disp));
    }, 0};
}

// fild dword ptr [base + disp]  (integer-to-float load)
inline TrampolineStepHandle fildMem(Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.fild(asmjit::x86::dword_ptr(detail::wildAbiMapReg(base), disp));
    }, 0};
}

// fld qword ptr [base + disp]  (double-precision load)
inline TrampolineStepHandle fldMemDouble(Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.fld(asmjit::x86::qword_ptr(detail::wildAbiMapReg(base), disp));
    }, 0};
}

// fstp dword ptr [base + disp]  (single-precision pop-store)
inline TrampolineStepHandle fstpMem(Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.fstp(asmjit::x86::dword_ptr(detail::wildAbiMapReg(base), disp));
    }, 0};
}

// fstp qword ptr [base + disp]  (double-precision pop-store)
inline TrampolineStepHandle fstpMemDouble(Reg base, std::int32_t disp = 0) {
    return TrampolineStepHandle{[base, disp](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.fstp(asmjit::x86::qword_ptr(detail::wildAbiMapReg(base), disp));
    }, 0};
}

// call <addr>
inline TrampolineStepHandle callTo(std::uintptr_t addr, std::int32_t clean_bytes = 0) {
    return TrampolineStepHandle{[addr, clean_bytes](asmjit::x86::Assembler& a, const asmjit::x86::Gp& scratch) {
        a.mov(scratch, asmjit::imm(addr));
        a.call(scratch);
        if (clean_bytes > 0) { a.add(asmjit::x86::esp, clean_bytes); }
    }, -clean_bytes};
}

// call <reg>
inline TrampolineStepHandle callReg(Reg reg, std::int32_t clean_bytes = 0) {
    return TrampolineStepHandle{[reg, clean_bytes](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.call(detail::wildAbiMapReg(reg));
        if (clean_bytes > 0) { a.add(asmjit::x86::esp, clean_bytes); }
    }, -clean_bytes};
}

// raw escape hatch
template <typename Fn>
TrampolineStepHandle raw(Fn&& fn, std::int32_t push_delta = 0) {
    return TrampolineStepHandle{std::forward<Fn>(fn), push_delta};
}

// raw() for a callable that takes only the Assembler
template <typename Fn>
requires std::is_invocable_v<Fn, asmjit::x86::Assembler&> TrampolineStepHandle rawA(
    Fn&& fn, std::int32_t push_delta = 0) {
    return raw([f = std::forward<Fn>(fn)](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { f(a); }, push_delta);
}

class TrampolineEpilogue {
public:
    template <typename Fn>
    requires(!std::is_same_v<std::decay_t<Fn>, TrampolineEpilogue>) explicit TrampolineEpilogue(Fn&& fn)
        : emit_(std::forward<Fn>(fn)) {}

    void emit(asmjit::x86::Assembler& a, const asmjit::x86::Gp& scratch) const { emit_(a, scratch); }

private:
    std::function<void(asmjit::x86::Assembler&, const asmjit::x86::Gp&)> emit_;
};

namespace detail {
using SharedStepList = std::shared_ptr<const std::vector<TrampolineStepHandle>>;

template <typename... Steps>
SharedStepList makeSharedStepList(Steps&&... steps) {
    auto list = std::make_shared<std::vector<TrampolineStepHandle>>();
    list->reserve(sizeof...(Steps));
    (list->push_back(std::forward<Steps>(steps)), ...);
    return list;
}

inline void emitStepList(const SharedStepList& list, asmjit::x86::Assembler& a, const asmjit::x86::Gp& scratch) {
    if (!list) { return; }
    for (const auto& s : *list) {
        s.emit(a, scratch);
    }
}
}  // namespace detail

template <typename... T>
concept TrampolineSteps = (std::is_same_v<std::decay_t<T>, TrampolineStepHandle> && ...);

using Cond = asmjit::x86::CondCode;

enum class ResultWidth { eByte, eWord, eDword };

namespace detail {
inline asmjit::x86::Gp resultReg(ResultWidth w) {
    switch (w) {
        case ResultWidth::eByte:
            return asmjit::x86::al;
        case ResultWidth::eWord:
            return asmjit::x86::ax;
        case ResultWidth::eDword:
            break;
    }
    return asmjit::x86::eax;
}

// kept as data rather than a callable so the lambda below captures only trivially-copyable state
enum class FlagSource { eNone, eTest, eCmp };

struct FlagSetter {
    FlagSource kind = FlagSource::eNone;
    ResultWidth width = ResultWidth::eDword;
    std::int32_t value = 0;
};

inline void emitFlagSetter(const FlagSetter& fs, asmjit::x86::Assembler& a) {
    const asmjit::x86::Gp result = resultReg(fs.width);
    switch (fs.kind) {
        case FlagSource::eNone:
            break;
        case FlagSource::eTest:
            a.test(result, result);
            break;
        case FlagSource::eCmp:
            a.cmp(result, asmjit::imm(fs.value));
            break;
    }
}

// shared shape of every branching epilogue: set the flags (or don't), then the
// taken arm falls through and the other one is jumped to
class BranchEmitter {
public:
    BranchEmitter(
        const FlagSetter& pre, Cond cc, TrampolineEpilogue taken, TrampolineEpilogue untaken, SharedStepList shared)
        : pre_(pre), cc_(cc), taken_(std::move(taken)), untaken_(std::move(untaken)), shared_(std::move(shared)) {}

    void operator()(asmjit::x86::Assembler& a, const asmjit::x86::Gp& scratch) const {
        emitFlagSetter(pre_, a);
        asmjit::Label else_arm = a.new_label();
        a.j(asmjit::x86::negate_cond(cc_), else_arm);
        emitStepList(shared_, a, scratch);
        taken_.emit(a, scratch);
        a.bind(else_arm);
        emitStepList(shared_, a, scratch);
        untaken_.emit(a, scratch);
    }

private:
    FlagSetter pre_;
    Cond cc_;
    TrampolineEpilogue taken_;
    TrampolineEpilogue untaken_;
    SharedStepList shared_;
};

inline TrampolineEpilogue branchImpl(
    const FlagSetter& pre, Cond cc, TrampolineEpilogue taken, TrampolineEpilogue untaken, SharedStepList shared) {
    return TrampolineEpilogue{BranchEmitter{pre, cc, std::move(taken), std::move(untaken), std::move(shared)}};
}
}  // namespace detail

// steps emitted by an epilogue do not count toward the builder's pushedBytes()
template <typename... Steps>
    requires(sizeof...(Steps) > 0) &&
    TrampolineSteps<Steps...> TrampolineEpilogue withSteps(TrampolineEpilogue ep, Steps&&... steps) {
    return TrampolineEpilogue{[ep = std::move(ep), list = detail::makeSharedStepList(std::forward<Steps>(steps)...)](
                                  asmjit::x86::Assembler& a, const asmjit::x86::Gp& scratch) {
        detail::emitStepList(list, a, scratch);
        ep.emit(a, scratch);
    }};
}

// jmp <fixed address>, via push imm32
inline TrampolineEpilogue jmpTo(std::uintptr_t addr) {
    return TrampolineEpilogue{[addr](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) {
        a.push(asmjit::imm(addr));
        a.ret();
    }};
}

// jmp <fixed address> after the given steps; a flag-setting replay reaches `addr` with its flags intact
template <typename... Steps>
    requires(sizeof...(Steps) > 0) &&
    TrampolineSteps<Steps...> TrampolineEpilogue jmpTo(std::uintptr_t addr, Steps&&... steps) {
    return withSteps(jmpTo(addr), std::forward<Steps>(steps)...);
}

// jmp <reg>
inline TrampolineEpilogue jmpReg(Reg reg) {
    return TrampolineEpilogue{
        [reg](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.jmp(detail::wildAbiMapReg(reg)); }};
}

template <typename... Steps>
    requires(sizeof...(Steps) > 0) && TrampolineSteps<Steps...> TrampolineEpilogue jmpReg(Reg reg, Steps&&... steps) {
    return withSteps(jmpReg(reg), std::forward<Steps>(steps)...);
}

// ret
inline TrampolineEpilogue ret() {
    return TrampolineEpilogue{[](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.ret(); }};
}

template <typename... Steps>
    requires(sizeof...(Steps) > 0) && TrampolineSteps<Steps...> TrampolineEpilogue ret(Steps&&... steps) {
    return withSteps(ret(), std::forward<Steps>(steps)...);
}

// ret <bytes>
inline TrampolineEpilogue retN(std::int32_t bytes) {
    return TrampolineEpilogue{[bytes](asmjit::x86::Assembler& a, const asmjit::x86::Gp&) { a.ret(bytes); }};
}

template <typename... Steps>
    requires(sizeof...(Steps) > 0) &&
    TrampolineSteps<Steps...> TrampolineEpilogue retN(std::int32_t bytes, Steps&&... steps) {
    return withSteps(retN(bytes), std::forward<Steps>(steps)...);
}

template <typename... Steps>
requires TrampolineSteps<Steps...> TrampolineEpilogue branchOnFlags(
    Cond cc, TrampolineEpilogue taken, TrampolineEpilogue untaken, Steps&&... shared) {
    return detail::branchImpl(detail::FlagSetter{}, cc, std::move(taken), std::move(untaken),
        detail::makeSharedStepList(std::forward<Steps>(shared)...));
}

// test <result>, <result>; then nonzero -> `nonzero`, zero -> `zero`.
template <typename... Steps>
requires TrampolineSteps<Steps...> TrampolineEpilogue branchOnResult(
    ResultWidth width, TrampolineEpilogue nonzero, TrampolineEpilogue zero, Steps&&... shared) {
    return detail::branchImpl(detail::FlagSetter{detail::FlagSource::eTest, width}, Cond::kNotZero, std::move(nonzero),
        std::move(zero), detail::makeSharedStepList(std::forward<Steps>(shared)...));
}

template <typename... Steps>
requires TrampolineSteps<Steps...> TrampolineEpilogue branchOnResult(
    TrampolineEpilogue nonzero, TrampolineEpilogue zero, Steps&&... shared) {
    return branchOnResult(ResultWidth::eDword, std::move(nonzero), std::move(zero), std::forward<Steps>(shared)...);
}

// cmp <result>, value; then `cc` read against that comparison
// (Cond::kEqual, Cond::kSignedLT, Cond::kUnsignedGE, ...).
template <typename... Steps>
requires TrampolineSteps<Steps...> TrampolineEpilogue branchOnResultCmp(ResultWidth width, Cond cc, std::int32_t value,
    TrampolineEpilogue taken, TrampolineEpilogue untaken, Steps&&... shared) {
    return detail::branchImpl(detail::FlagSetter{detail::FlagSource::eCmp, width, value}, cc, std::move(taken),
        std::move(untaken), detail::makeSharedStepList(std::forward<Steps>(shared)...));
}

template <typename... Steps>
requires TrampolineSteps<Steps...> TrampolineEpilogue branchOnResultCmp(
    Cond cc, std::int32_t value, TrampolineEpilogue taken, TrampolineEpilogue untaken, Steps&&... shared) {
    return branchOnResultCmp(
        ResultWidth::eDword, cc, value, std::move(taken), std::move(untaken), std::forward<Steps>(shared)...);
}

struct ResultCase {
    std::int32_t value;
    TrampolineEpilogue epilogue;
};

template <typename... Steps>
requires TrampolineSteps<Steps...> TrampolineEpilogue switchOnResult(
    ResultWidth width, std::vector<ResultCase> cases, TrampolineEpilogue fallback, Steps&&... shared) {
    return TrampolineEpilogue{
        [width, cases = std::make_shared<const std::vector<ResultCase>>(std::move(cases)),
            fallback = std::move(fallback), list = detail::makeSharedStepList(std::forward<Steps>(shared)...)](
            asmjit::x86::Assembler& a, const asmjit::x86::Gp& scratch) {
        const asmjit::x86::Gp result = detail::resultReg(width);
        std::vector<asmjit::Label> arms;
        arms.reserve(cases->size());
        for (const auto& [value, epilogue] : *cases) {
            arms.push_back(a.new_label());
            a.cmp(result, asmjit::imm(value));
            a.je(arms.back());
        }
        detail::emitStepList(list, a, scratch);
        fallback.emit(a, scratch);
        for (std::size_t i = 0; i < cases->size(); ++i) {
            a.bind(arms[i]);
            detail::emitStepList(list, a, scratch);
            (*cases)[i].epilogue.emit(a, scratch);
        }
    }};
}

template <typename... Steps>
requires TrampolineSteps<Steps...> TrampolineEpilogue switchOnResult(
    std::vector<ResultCase> cases, TrampolineEpilogue fallback, Steps&&... shared) {
    return switchOnResult(ResultWidth::eDword, std::move(cases), std::move(fallback), std::forward<Steps>(shared)...);
}

// escape hatch for anything else
template <typename Fn>
TrampolineEpilogue rawEpilogue(Fn&& fn) {
    return TrampolineEpilogue{std::forward<Fn>(fn)};
}

class CallsiteTrampolineBuilder {
public:
    static constexpr std::int32_t kAutoArgBytes = -1;

    CallsiteTrampolineBuilder() = default;
    ~CallsiteTrampolineBuilder() = default;

    CallsiteTrampolineBuilder(const CallsiteTrampolineBuilder&) = delete;
    CallsiteTrampolineBuilder& operator=(const CallsiteTrampolineBuilder&) = delete;
    CallsiteTrampolineBuilder(CallsiteTrampolineBuilder&&) = default;
    CallsiteTrampolineBuilder& operator=(CallsiteTrampolineBuilder&&) = default;

    CallsiteTrampolineBuilder& scratchReg(Reg r) {
        assert(r != Reg::eSp &&
            "hookkit: CallsiteTrampolineBuilder::scratchReg: esp can't be used as a scratch register, the call and "
            "the steps would overwrite the stack pointer");
        scratch_reg_ = r;
        return *this;
    }

    CallsiteTrampolineBuilder& scratchStack(std::int32_t bytes) {
        assert(bytes >= 0 &&
            "hookkit: CallsiteTrampolineBuilder::scratchStack: negative size, build() would silently reserve nothing");
        scratch_bytes_ = bytes;
        return *this;
    }

    CallsiteTrampolineBuilder& step(TrampolineStepHandle s) {
        pushed_bytes_ += s.pushDelta();
        steps_.push_back(std::move(s));
        return *this;
    }

    CallsiteTrampolineBuilder& assertOnBuildFailure(bool enabled = true) {
        assert_on_failure_ = enabled;
        return *this;
    }

    [[nodiscard]]
    std::int32_t pushedBytes() const {
        return pushed_bytes_;
    }

    // JIT-compiles the trampoline: the optional scratch reservation, the steps in order, a call to target_addr, stack
    // cleanup (arg_bytes, then the scratch reservation) and the epilogue. target_addr's call is always the last one;
    // chain earlier calls as callTo()/callReg() steps.
    //
    // epilogue: jmpTo(addr) resumes at a fixed address (a patched call site: HookType::target("name") or
    //   HookType::targetByKey<K>()), jmpReg(reg) at one a step left in a register, ret()/retN(bytes) when replacing a
    //   call target rather than a call site, rawEpilogue(fn) for anything else (multi-way exits, conditional branches)
    // arg_bytes: stack bytes cleaned after the call; defaults to pushedBytes(), right for a cdecl callee taking exactly
    //   the pushed steps. 0 when the callee cleans the stack (stdcall), less when only some pushed bytes are arguments
    // returns null on JIT failure
    [[nodiscard]]
    void* build(
        std::uintptr_t target_addr, const TrampolineEpilogue& epilogue, std::int32_t arg_bytes = kAutoArgBytes) const {
        const std::int32_t clean_bytes = (arg_bytes == kAutoArgBytes) ? pushed_bytes_ : arg_bytes;
        return assemble(target_addr, clean_bytes, epilogue);
    }

    // JIT-compile a call-free trampoline, no auto-cleanup
    [[nodiscard]]
    void* build(const TrampolineEpilogue& epilogue) const {
        return assemble(std::nullopt, 0, epilogue);
    }

private:
    [[nodiscard]]
    void* assemble(
        std::optional<std::uintptr_t> target_addr, std::int32_t clean_bytes, const TrampolineEpilogue& epilogue) const {
        asmjit::CodeHolder code;
        if (const asmjit::Error init_err = code.init(detail::wild_abi_jit_runtime->environment());
            init_err != asmjit::kErrorOk) {
            assert(!assert_on_failure_ &&
                "hookkit: CallsiteTrampolineBuilder::build: CodeHolder::init failed, init_err is the asmjit::Error");
            return nullptr;
        }
        detail::JitErrorHandler err_handler;
        code.set_error_handler(&err_handler);
        asmjit::x86::Assembler a(&code);

        const asmjit::x86::Gp scratch = detail::wildAbiMapReg(scratch_reg_);

        if (scratch_bytes_ > 0) { a.sub(asmjit::x86::esp, scratch_bytes_); }
        for (const auto& s : steps_) {
            s.emit(a, scratch);
        }

        if (target_addr) {
            a.mov(scratch, asmjit::imm(*target_addr));
            a.call(scratch);
        }

        if (clean_bytes > 0) { a.add(asmjit::x86::esp, clean_bytes); }
        if (scratch_bytes_ > 0) { a.add(asmjit::x86::esp, scratch_bytes_); }

        epilogue.emit(a, scratch);

        if (err_handler.hadError()) {
            assert(!assert_on_failure_ &&
                "hookkit: CallsiteTrampolineBuilder::build: asmjit rejected an emit from a step or the epilogue, break "
                "in detail::JitErrorHandler::handle_error for its error and message");
            return nullptr;
        }

        void* fn = nullptr;
        if (const asmjit::Error add_err = detail::wild_abi_jit_runtime->add(&fn, &code); add_err != asmjit::kErrorOk) {
            assert(!assert_on_failure_ &&
                "hookkit: CallsiteTrampolineBuilder::build: JitRuntime::add failed, add_err is the asmjit::Error");
            return nullptr;
        }
        return fn;
    }

    std::vector<TrampolineStepHandle> steps_;
    std::int32_t scratch_bytes_{0};
    std::int32_t pushed_bytes_{0};
    bool assert_on_failure_{false};
    Reg scratch_reg_{Reg::eAx};
};

// eax/ecx/edx are clobbered exactly as the original call would; one trampoline is JIT-built per Fn
template <typename Tag, std::uintptr_t Site, typename Callee>
struct CallsiteHook : Hook<Tag, Site, Conv::eCdecl, void> {
    using CalleeHook = Callee;
    static constexpr std::uintptr_t kResume = Site + 5;

    template <const auto& Fn>
    static void* staticDetour() {
        [[maybe_unused]] static const bool verified = verifySite();
        static void* const built = CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(
            reinterpret_cast<std::uintptr_t>(&Callee::Descriptor::template detour<Fn>), jmpTo(kResume));
        return built;
    }

private:
    static bool verifySite() {
        if (CallsiteHook::attached.load(std::memory_order_acquire)) { return true; }  // the site holds MinHook's jmp
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(Site);
        std::int32_t rel = 0;
        std::memcpy(&rel, bytes + 1, sizeof(rel));
        const bool ok = bytes[0] == 0xE8 && kResume + static_cast<std::uintptr_t>(rel) == Callee::kAddress;
        assert(ok && "hookkit: CallsiteHook: Site is not a `call rel32` to Callee::kAddress");
        return ok;
    }
};

/**
 * @brief Defines a zero-sized CallsiteHook handle: the `call rel32` at ADDR, whose callee is the hook handle CALLEE.
 *        Variants come from NAME::staticDetour<Fn>(), Fn having CALLEE's signature.
 * @code
 *   HOOKKIT_CALLSITE_HOOK(fetch_call, 0x..., CTexture::fetchGxTex);
 *   reinstall(fetch_call{}, fetch_call::staticDetour<kFetchVariant<true>>());
 * @endcode
 */
#define HOOKKIT_CALLSITE_HOOK(NAME, ADDR, CALLEE)                         \
    struct NAME##_tag {};                                                 \
    struct NAME : ::hookkit::CallsiteHook<NAME##_tag, (ADDR), CALLEE> {}; \
    using NAME##_hook = NAME;                                             \
    static_assert(std::is_empty_v<NAME>, #NAME " must stay zero-sized" HOOKKIT_ZERO_SIZED_WHY_)

}  // namespace hookkit
