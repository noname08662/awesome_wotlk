#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef HOOKKIT_DETACH_DRAIN_TIMEOUT_MS
#define HOOKKIT_DETACH_DRAIN_TIMEOUT_MS 5000
#endif

// how long a transaction may wait for the lock and retry transient failures before it gives up
#ifndef HOOKKIT_DEFAULT_RETRY_BUDGET_MS
#define HOOKKIT_DEFAULT_RETRY_BUDGET_MS 250
#endif

// fires on every non-transient failure (a caller bug: double attach, detach of an unattached hook, cap overflow, ...)
#ifndef HOOKKIT_ASSERT
#define HOOKKIT_ASSERT(expr, msg) assert((expr) && (msg))
#endif

// test seam: evaluated at each detail::CommitStage of a commit attempt, anything but NO_ERROR fails the attempt there
#ifndef HOOKKIT_COMMIT_FAULT
#define HOOKKIT_COMMIT_FAULT(stage) NO_ERROR
#endif

#include <MinHook.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <mutex>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "accessor.h"
#include "patch.h"

namespace hookkit {

namespace detail {

// MH_STATUS -> Win32-style LONG
inline LONG mapMinHookStatus(MH_STATUS st) {
    switch (st) {
        case MH_OK:
            return NO_ERROR;
        case MH_ERROR_ALREADY_INITIALIZED:
            return NO_ERROR;
        case MH_ERROR_NOT_INITIALIZED:
            return ERROR_NOT_READY;
        case MH_ERROR_ALREADY_CREATED:
            return ERROR_ALREADY_EXISTS;
        case MH_ERROR_NOT_CREATED:
            return ERROR_NOT_FOUND;
        case MH_ERROR_ENABLED:
            return ERROR_ALREADY_EXISTS;
        case MH_ERROR_DISABLED:
            return ERROR_NOT_FOUND;
        case MH_ERROR_NOT_EXECUTABLE:
            return ERROR_INVALID_ADDRESS;
        case MH_ERROR_UNSUPPORTED_FUNCTION:
            return ERROR_NOT_SUPPORTED;
        case MH_ERROR_MEMORY_ALLOC:
            return ERROR_NOT_ENOUGH_MEMORY;
        case MH_ERROR_MEMORY_PROTECT:
            return ERROR_NOACCESS;
        case MH_ERROR_MODULE_NOT_FOUND:
            return ERROR_MOD_NOT_FOUND;
        case MH_ERROR_FUNCTION_NOT_FOUND:
            return ERROR_PROC_NOT_FOUND;
        case MH_UNKNOWN:
        default:
            return ERROR_GEN_FAILURE;
    }
}

enum class CommitStage { eBeforeApply, eAfterHooks, eAfterPatches };

inline bool isTransientStatus(LONG status) {
    switch (status) {
        case ERROR_TIMEOUT:
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_NOACCESS:
        case ERROR_NOT_READY:
        case ERROR_GEN_FAILURE:
            return true;
        default:
            return false;
    }
}

inline constinit utils::Accessor<std::timed_mutex, struct TransactionMutexTag> transaction_mutex{};

inline constinit utils::Accessor<std::atomic<DWORD>, struct TransactionOwnerTag> transaction_owner{};

inline constinit utils::Accessor<std::atomic<bool>, struct TransactionPendingTag> transaction_pending{};

inline constinit utils::Accessor<std::atomic<bool>, struct TransactionJoinedTag> transaction_joined{};

inline constinit utils::Accessor<std::mutex, struct MhMapMutexTag> mh_map_mutex{};

inline constinit utils::Accessor<std::unordered_map<void*, void*>, struct MhOriginalByTrampolineTag> mh_original_by_trampoline{};

inline LONG mhLazyInitialize() {
    static bool initialized = false;
    if (initialized) { return NO_ERROR; }
    const LONG status = mapMinHookStatus(MH_Initialize());
    initialized = status == NO_ERROR;
    return status;
}

struct MhPendingOp {
    void* addr;
    void* trampoline;
    PVOID* slot;
    bool is_detach;
    void (*drain)() = nullptr;
};

inline void flushQuiesce() { FlushProcessWriteBuffers(); }

inline constinit utils::Accessor<std::mutex, struct MhPendingOpsMutexTag> mh_pending_ops_mutex{};

inline constinit utils::Accessor<std::vector<MhPendingOp>, struct MhPendingOpsTag> mh_pending_ops{};

inline void pushMhPendingOp(const MhPendingOp& op) {
    std::lock_guard lk(*mh_pending_ops_mutex);
    mh_pending_ops->push_back(op);
}

extern "C" {
#include "MinHook/src/hde/hde32.h"
#include "MinHook/src/trampoline.h"
}

struct IpBoundaryInfo {
    void* target = nullptr;
    void* trampoline = nullptr;
    UINT n_ip = 0;
    UINT8 old_ips[8]{};
    UINT8 new_ips[8]{};
    bool prologue_has_call = true;
};

inline constexpr std::size_t kIpBoundaryScratchSize = 64;

inline IpBoundaryInfo buildIpBoundaryInfo(void* target, void* trampoline) {
    IpBoundaryInfo info;
    info.target = target;
    info.trampoline = trampoline;

    void* scratch = VirtualAlloc(nullptr, kIpBoundaryScratchSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (scratch == nullptr) { return info; }

    TRAMPOLINE ct{};
    ct.pTarget = target;
    ct.pTrampoline = scratch;
    ct.pDetour = nullptr;

    if (CreateTrampolineFunction(&ct) != 0) {
        info.n_ip = ct.nIP;
        std::memcpy(info.old_ips, ct.oldIPs, sizeof(info.old_ips));
        std::memcpy(info.new_ips, ct.newIPs, sizeof(info.new_ips));

        bool has_call = false;
        for (UINT i = 0; i < ct.nIP && !has_call; ++i) {
            const UINT off = ct.oldIPs[i];
            if (off >= sizeof(JMP_REL)) { break; }
            hde32s hs{};
            hde32_disasm(static_cast<const std::uint8_t*>(target) + off, &hs);
            if ((hs.flags & F_ERROR) != 0 || hs.opcode == 0xE8 || hs.opcode == 0x9A ||
                (hs.opcode == 0xFF && (hs.modrm_reg == 2 || hs.modrm_reg == 3))) {
                has_call = true;
            }
        }
        info.prologue_has_call = has_call;
    }
    VirtualFree(scratch, 0, MEM_RELEASE);
    return info;
}

inline DWORD_PTR translateIp(const IpBoundaryInfo& info, DWORD_PTR ip) {
    const auto target_base = reinterpret_cast<DWORD_PTR>(info.target);
    for (UINT i = 0; i < info.n_ip; ++i) {
        if (ip == target_base + info.old_ips[i]) {
            return reinterpret_cast<DWORD_PTR>(info.trampoline) + info.new_ips[i];
        }
    }
    return 0;
}

inline constinit utils::Accessor<std::unordered_map<void*, IpBoundaryInfo>, struct IpBoundaryByTargetTag> ip_boundary_by_target{};

inline void runMhPostApplySuccess() {
    std::lock_guard lk(*mh_pending_ops_mutex);
    auto& ops = *mh_pending_ops;
    for (auto& op : ops) {
        if (op.is_detach) {
            MH_RemoveHook(op.addr);
            std::lock_guard map_lk(*mh_map_mutex);
            mh_original_by_trampoline->erase(op.trampoline);
            ip_boundary_by_target->erase(op.addr);
        }
    }
    ops.clear();
}

struct GuardedPage {
    std::uintptr_t page_base;
    DWORD original_protect;
};

inline constexpr std::uintptr_t kPageGuardPageSize = 4096;
inline constexpr std::uintptr_t kPageGuardPageMask = ~(kPageGuardPageSize - 1);

inline constexpr DWORD kPageGuardWaitTimeoutMs = 5000;

inline constinit utils::Accessor<std::mutex, struct PageGuardMutexTag> page_guard_mutex{};

struct PageGuardState {
    std::vector<std::uintptr_t> guarded_pages{};
    std::vector<void*> guarded_targets{};
    DWORD committing_thread_id = 0;
};

inline constinit utils::Accessor<PageGuardState, struct PageGuardStateTag> page_guard_state{};

inline HANDLE createPageGuardReleaseEvent() { return CreateEventW(nullptr, TRUE, FALSE, nullptr); }

inline constinit utils::Accessor<HANDLE, struct PageGuardReleaseEventTag, &createPageGuardReleaseEvent> page_guard_release_event{};

inline void pageGuardHomeModuleAnchor() {}

inline HMODULE queryPageGuardHomeModule() {
    HMODULE h = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&pageGuardHomeModuleAnchor), &h);
    return h;
}

inline constinit utils::Accessor<HMODULE, struct PageGuardHomeModuleTag, &queryPageGuardHomeModule> page_guard_home_module{};

inline LONG WINAPI pageGuardVeh(EXCEPTION_POINTERS* info) {
    if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) { return EXCEPTION_CONTINUE_SEARCH; }
    const auto fault_addr = static_cast<std::uintptr_t>(info->ExceptionRecord->ExceptionInformation[1]);
    const std::uintptr_t fault_page = fault_addr & kPageGuardPageMask;

    std::vector<IpBoundaryInfo> boundary_snapshot;
    {
        std::lock_guard lk(*page_guard_mutex);
        const auto& [guarded_pages, guarded_targets, committing_thread_id] = *page_guard_state;

        if (GetCurrentThreadId() == committing_thread_id) { return EXCEPTION_CONTINUE_SEARCH; }
        bool guarded = false;
        for (std::uintptr_t page : guarded_pages) {
            if (page == fault_page) {
                guarded = true;
                break;
            }
        }
        if (!guarded) { return EXCEPTION_CONTINUE_SEARCH; }

        std::lock_guard map_lk(*mh_map_mutex);
        auto& map = *ip_boundary_by_target;
        for (void* target : guarded_targets) {
            auto it = map.find(target);
            if (it != map.end()) { boundary_snapshot.push_back(it->second); }
        }
    }

    WaitForSingleObject(*page_guard_release_event, kPageGuardWaitTimeoutMs);

    for (const auto& info_entry : boundary_snapshot) {
        const DWORD_PTR translated = translateIp(info_entry, info->ContextRecord->Eip);
        if (translated != 0) {
            info->ContextRecord->Eip = static_cast<DWORD>(translated);
            break;
        }
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

inline bool pageGuardIsSystemModule(HMODULE h) {
    if (h == nullptr) { return false; }
    wchar_t module_path[MAX_PATH];
    const DWORD path_len = GetModuleFileNameW(h, module_path, MAX_PATH);
    if (path_len == 0 || path_len == MAX_PATH) { return false; }
    wchar_t system_dir[MAX_PATH];
    const UINT dir_len = GetSystemDirectoryW(system_dir, MAX_PATH);
    if (dir_len == 0 || dir_len >= MAX_PATH) { return false; }
    return path_len > dir_len && module_path[dir_len] == L'\\' && _wcsnicmp(module_path, system_dir, dir_len) == 0;
}

inline constinit utils::Accessor<void*, struct PageGuardVehHandleSlotTag> page_guard_veh_handle_slot{};

inline void installPageGuardVeh() {
    std::lock_guard lk(*page_guard_mutex);
    void*& handle = *page_guard_veh_handle_slot;
    if (handle == nullptr) { handle = AddVectoredExceptionHandler(1, &pageGuardVeh); }
}

inline void removePageGuardVeh() {
    std::lock_guard lk(*page_guard_mutex);
    void*& handle = *page_guard_veh_handle_slot;
    if (handle != nullptr) {
        RemoveVectoredExceptionHandler(handle);
        handle = nullptr;
    }
}

class PageGuardSession {
public:
    explicit PageGuardSession(const std::vector<void*>& addrs) {
        installPageGuardVeh();
        ResetEvent(*page_guard_release_event);

        std::lock_guard lk(*page_guard_mutex);
        auto& state = *page_guard_state;
        state.committing_thread_id = GetCurrentThreadId();
        state.guarded_targets = addrs;
        for (void* a : addrs) {
            const std::uintptr_t base = reinterpret_cast<std::uintptr_t>(a);
            constexpr std::uintptr_t kPatchRegionBefore = 5;
            constexpr std::uintptr_t kPatchRegionAfterExclusive = 16;
            const std::uintptr_t region_end = base + kPatchRegionAfterExclusive;
            std::uintptr_t page = (base - kPatchRegionBefore) & kPageGuardPageMask;
            do {
                guardPageLocked(page, state);
                page += kPageGuardPageSize;
            } while (page < region_end);
        }
    }

    ~PageGuardSession() {
        {
            std::lock_guard lk(*page_guard_mutex);
            for (const auto& [page_base, original_protect] : pages_) {
                DWORD ignored = 0;
                VirtualProtect(reinterpret_cast<void*>(page_base), kPageGuardPageSize, original_protect, &ignored);
            }
            auto& [guarded_pages, guarded_targets, committing_thread_id] = *page_guard_state;
            guarded_pages.clear();
            guarded_targets.clear();
            committing_thread_id = 0;
        }

        SetEvent(*page_guard_release_event);
    }

    PageGuardSession(const PageGuardSession&) = delete;
    PageGuardSession& operator=(const PageGuardSession&) = delete;
    PageGuardSession(PageGuardSession&&) = delete;
    PageGuardSession& operator=(PageGuardSession&&) = delete;

private:
    void guardPageLocked(std::uintptr_t page_base, PageGuardState& state) {
        for (const auto& [existing_base, existing_protect] : pages_) {
            if (existing_base == page_base) { return; }
        }

        HMODULE page_module = nullptr;
        const BOOL resolved =
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(page_base), &page_module);
        if (resolved != 0) {
            if (page_module == *page_guard_home_module) { return; }
            if (pageGuardIsSystemModule(page_module)) { return; }
        }

        DWORD old_protect = 0;
        if (VirtualProtect(reinterpret_cast<void*>(page_base), kPageGuardPageSize, PAGE_NOACCESS, &old_protect) == 0) {
            return;
        }
        pages_.push_back({page_base, old_protect});
        state.guarded_pages.push_back(page_base);
    }

    std::vector<GuardedPage> pages_{};
};

inline constexpr int kAbortRemoveAttempts = 3;

inline void runMhAbort() {
    std::lock_guard lk(*mh_pending_ops_mutex);
    auto& ops = *mh_pending_ops;
    std::vector<void*> reenabled;
    for (auto it = ops.rbegin(); it != ops.rend(); ++it) {
        if (it->is_detach) {
            MH_QueueEnableHook(it->addr);
            if (it->slot) { *it->slot = it->trampoline; }
            reenabled.push_back(it->addr);
        } else {
            // a commit that failed after MH_ApplyQueued left this jump live: take it out first, then let callers
            // already inside the detour leave through the trampoline, which stays valid until the removal
            MH_DisableHook(it->addr);
            if (it->drain) { it->drain(); }
            MH_STATUS removed = MH_RemoveHook(it->addr);
            for (int attempt = 1; removed != MH_OK && removed != MH_ERROR_NOT_CREATED && attempt < kAbortRemoveAttempts;
                ++attempt) {
                Sleep(1);
                removed = MH_RemoveHook(it->addr);
            }
            if (removed != MH_OK && removed != MH_ERROR_NOT_CREATED) {
                // the jump is still live: the slot must keep pointing at the trampoline, or calling "the original"
                // re-enters the detour forever
                HOOKKIT_ASSERT(removed == MH_OK || removed == MH_ERROR_NOT_CREATED,
                    "hookkit: abort could not remove a hook, it stays live -- MH_RemoveHook kept failing (removed is "
                    "the MH_STATUS), so the slot keeps pointing at the trampoline");
                continue;
            }
            if (it->slot) { *it->slot = it->addr; }
            std::lock_guard map_lk(*mh_map_mutex);
            mh_original_by_trampoline->erase(it->trampoline);
            ip_boundary_by_target->erase(it->addr);
        }
    }
    ops.clear();

    // a commit that failed halfway through MH_ApplyQueued already disabled some of the detached hooks; re-queueing
    // alone would leave them marked attached but unpatched until some later transaction applies the queue
    if (!reenabled.empty()) {
        MH_STATUS reapplied = MH_OK;
        for (int attempt = 0; attempt < kAbortRemoveAttempts; ++attempt) {
            PageGuardSession guard(reenabled);
            reapplied = MH_ApplyQueued();
            if (reapplied == MH_OK) { break; }
        }
        HOOKKIT_ASSERT(reapplied == MH_OK,
            "hookkit: abort could not re-enable detached hooks -- MH_ApplyQueued kept failing (reapplied is the "
            "MH_STATUS), they stay marked attached but unpatched until a later transaction applies the queue");
    }
}

}  // namespace detail

struct RetryBudget {
    DWORD ms = HOOKKIT_DEFAULT_RETRY_BUDGET_MS;
};

class HookTransaction {
public:
    enum class Mode { eBeginNew, eAttachToExisting };

    explicit HookTransaction(Mode mode = Mode::eBeginNew, bool suspend_all_threads = true, RetryBudget budget = {})
        : owns_transaction_(mode == Mode::eBeginNew),
          suspend_all_threads_(suspend_all_threads),
          deadline_(GetTickCount64() + budget.ms) {
        if (!owns_transaction_) { return; }

        // the lock isn't recursive, so a second transaction on the owning thread could only fail or deadlock
        if (detail::transaction_owner->load(std::memory_order_acquire) == GetCurrentThreadId()) {
            fail(ERROR_INVALID_OPERATION);
            return;
        }
        status_ = acquireWithRetry();
    }

    explicit HookTransaction(RetryBudget budget) : HookTransaction(Mode::eBeginNew, true, budget) {}

    ~HookTransaction() {
        if (!committed_) {
            HOOKKIT_ASSERT(op_count_ == 0 && patches_.empty(),
                "hookkit: transaction with queued work was never committed -- call commit(), or abort() to drop it on "
                "purpose; the destructor rolls it back");
            if (started_) { detail::runMhAbort(); }
            runAbortActions();
            releaseTransactionLock();
        }
    }

    HookTransaction(const HookTransaction&) = delete;
    HookTransaction& operator=(const HookTransaction&) = delete;

    HookTransaction(HookTransaction&& other) noexcept
        : owns_transaction_(other.owns_transaction_),
          suspend_all_threads_(other.suspend_all_threads_),
          started_(other.started_),
          committed_(other.committed_),
          joined_(other.joined_),
          status_(other.status_),
          lock_held_(other.lock_held_),
          deadline_(other.deadline_),
          commit_actions_(other.commit_actions_),
          commit_action_count_(other.commit_action_count_),
          abort_actions_(other.abort_actions_),
          abort_action_count_(other.abort_action_count_),
          pending_touched_(other.pending_touched_),
          pending_touched_count_(other.pending_touched_count_),
          ops_(other.ops_),
          op_count_(other.op_count_),
          patches_(std::move(other.patches_)) {
        other.started_ = false;
        other.committed_ = true;
        other.commit_action_count_ = 0;
        other.abort_action_count_ = 0;
        other.pending_touched_count_ = 0;
        other.op_count_ = 0;
        other.patches_.clear();
        other.lock_held_ = false;
    }

    HookTransaction& operator=(HookTransaction&& other) noexcept {
        if (this != &other) {
            if (!committed_) {
                if (started_) { detail::runMhAbort(); }
                runAbortActions();
                releaseTransactionLock();
            }
            owns_transaction_ = other.owns_transaction_;
            suspend_all_threads_ = other.suspend_all_threads_;
            started_ = other.started_;
            committed_ = other.committed_;
            joined_ = other.joined_;
            status_ = other.status_;
            lock_held_ = other.lock_held_;
            deadline_ = other.deadline_;
            commit_actions_ = other.commit_actions_;
            commit_action_count_ = other.commit_action_count_;
            abort_actions_ = other.abort_actions_;
            abort_action_count_ = other.abort_action_count_;
            pending_touched_ = other.pending_touched_;
            pending_touched_count_ = other.pending_touched_count_;
            ops_ = other.ops_;
            op_count_ = other.op_count_;
            patches_ = std::move(other.patches_);
            other.started_ = false;
            other.committed_ = true;
            other.commit_action_count_ = 0;
            other.abort_action_count_ = 0;
            other.pending_touched_count_ = 0;
            other.op_count_ = 0;
            other.patches_.clear();
            other.lock_held_ = false;
        }
        return *this;
    }

    explicit operator bool() const noexcept { return status_ == NO_ERROR; }

    [[nodiscard]]
    LONG status() const noexcept {
        return status_;
    }

    [[nodiscard("on failure nothing was applied, the previous state persists")]]
    LONG commit() {
        if (committed_) { return status_; }
        committed_ = true;

        if (!owns_transaction_) {
            if (status_ == NO_ERROR) {
                runCommitActions();
            } else {
                runAbortActions();
            }
            return status_;
        }

        for (DWORD backoff_ms = 1;; backoff_ms = (std::min)(backoff_ms * 2, kMaxBackoffMs)) {
            if (status_ == NO_ERROR) {
                if (const LONG applied = applyQueued(); applied != NO_ERROR) { fail(applied); }
            }
            if (status_ == NO_ERROR) {
                detail::runMhPostApplySuccess();
                runCommitActions();
                break;
            }
            rollbackAttempt();
            if (!detail::isTransientStatus(status_) || joined_ || remainingMs() == 0) { break; }

            Sleep((std::min)(backoff_ms, remainingMs()));
            status_ = acquireWithRetry();
            if (status_ == NO_ERROR) { replayOps(); }
        }
        releaseTransactionLock();
        started_ = false;
        return status_;
    }

    // drops everything queued so far and releases the lock; status() becomes ERROR_CANCELLED unless it already failed
    void abort() {
        if (committed_) { return; }
        committed_ = true;

        if (!owns_transaction_) {
            // the ops sit in the owner's MinHook queue and the owner still applies them
            HOOKKIT_ASSERT(false, "hookkit: a joined transaction cannot abort, abort the owning one");
            runAbortActions();
            return;
        }

        restorePatches();
        if (started_) { detail::runMhAbort(); }
        runAbortActions();
        releaseTransactionLock();
        started_ = false;
        commit_action_count_ = 0;
        abort_action_count_ = 0;
        pending_touched_count_ = 0;
        op_count_ = 0;
        patches_.clear();
        // not fail(): a cancel is deliberate, not a logic error
        if (status_ == NO_ERROR) { status_ = ERROR_CANCELLED; }
    }

    template <typename... HookTypes>
    HookTransaction& attach(const HookTypes&...) {
        static_assert((std::is_empty_v<HookTypes> && ...),
            "expected zero-sized hook handle types -- pass a HOOKKIT_*_HANDLE type or a NAME##_hook{}");
        (attachOne<HookTypes>(), ...);
        return *this;
    }

    template <typename... HookTypes>
    HookTransaction& detach(const HookTypes&...) {
        static_assert((std::is_empty_v<HookTypes> && ...),
            "expected zero-sized hook handle types -- pass a HOOKKIT_*_HANDLE type or a NAME##_hook{}");
        (detachOne<HookTypes>(), ...);
        return *this;
    }

    template <typename... HookTypes>
    static LONG attachNow(const HookTypes&... hooks) {
        HookTransaction tx;
        if (!tx) { return tx.status(); }
        tx.attach(hooks...);
        return tx.commit();
    }

    template <typename HookType>
    static LONG reinstall(const HookType& hook, void* detour) {
        static_assert(std::is_empty_v<HookType>,
            "expected a zero-sized hook handle type -- pass a HOOKKIT_*_HANDLE type or a NAME##_hook{}");
        void* const live = HookType::attached.load(std::memory_order_acquire) ? HookType::resolveDetour() : nullptr;
        if (live == detour) { return NO_ERROR; }
        if (live != nullptr) {
            if (const LONG detached = detachNow(hook); detached != NO_ERROR) { return detached; }
        }
        if (detour == nullptr) { return NO_ERROR; }
        void* const previous = HookType::raw_detour.exchange(detour, std::memory_order_acq_rel);
        const LONG attached = attachNow(hook);
        if (attached != NO_ERROR) {
            HookType::raw_detour.store(previous, std::memory_order_release);
            if (live != nullptr) { static_cast<void>(attachNow(hook)); }
        }
        return attached;
    }

    template <typename Addr, typename Detour>
    HookTransaction& attachRaw(Addr** target, Detour detour) {
        auto** const slot = reinterpret_cast<void**>(target);
        auto* const detour_ptr = reinterpret_cast<void*>(detour);
        const RecordedOp op{.run = [](HookTransaction& tx, const RecordedOp& self) {
            tx.doAttachRaw(self.slot, self.detour);
        }, .slot = slot, .detour = detour_ptr};
        if (record(op)) { doAttachRaw(slot, detour_ptr); }
        return *this;
    }

    template <typename Addr>
    HookTransaction& detachRaw(Addr** target) {
        auto** const slot = reinterpret_cast<void**>(target);
        const RecordedOp op{
            .run = [](HookTransaction& tx, const RecordedOp& self) { tx.doDetachRaw(self.slot); }, .slot = slot};
        if (record(op)) { doDetachRaw(slot); }
        return *this;
    }

    template <typename... HookTypes>
    static LONG detachNow(const HookTypes&... hooks) {
        HookTransaction tx;
        if (!tx) { return tx.status(); }
        tx.detach(hooks...);
        return tx.commit();
    }

    // written together with the hooks when the transaction commits and restored if it rolls back
    template <typename T>
    HookTransaction& patchBytes(void* address, const T* data, std::size_t size) {
        static_assert(
            std::is_trivially_copyable_v<T>, "patch data must be trivially copyable, it is copied as raw bytes");
        if (!owns_transaction_) {
            // the owning transaction applies MinHook's queue, a joined one has nowhere to put the bytes
            fail(ERROR_INVALID_OPERATION);
            return *this;
        }
        if (address == nullptr || data == nullptr || size == 0) {
            fail(ERROR_INVALID_PARAMETER);
            return *this;
        }
        const auto* bytes = reinterpret_cast<const std::uint8_t*>(data);
        patches_.push_back({.address = address, .bytes = {bytes, bytes + size}});
        return *this;
    }

    template <typename T>
    HookTransaction& forceWrite(std::uintptr_t address, const T& value) {
        return patchBytes(reinterpret_cast<void*>(address), &value, sizeof(T));
    }

private:
    struct RecordedOp {
        void (*run)(HookTransaction&, const RecordedOp&) = nullptr;
        void** slot = nullptr;
        void* detour = nullptr;
    };

    struct PendingPatch {
        void* address = nullptr;
        std::vector<std::uint8_t> bytes;
        std::vector<std::uint8_t> backup;
        bool applied = false;
    };

    // every failure goes through here: transient ones are retried by commit(), the rest are caller bugs
    void fail(LONG status) {
        HOOKKIT_ASSERT(detail::isTransientStatus(status),
            "hookkit: transaction logic error, status says which -- "
            "ERROR_ALREADY_EXISTS: the hook is already attached (or attached twice in this transaction), or its target "
            "is already hooked; "
            "ERROR_NOT_FOUND: detach of a hook that isn't attached (or was already touched in this transaction), or of "
            "a slot that holds no hookkit trampoline; "
            "ERROR_INVALID_OPERATION: a second transaction on the thread that owns one, attach/detach in a joined "
            "transaction while none is open, or patchBytes/forceWrite on a joined transaction; "
            "ERROR_INVALID_PARAMETER: patchBytes with a null address/data or a zero size; "
            "ERROR_INVALID_FUNCTION: the hook has no detour (a WildHook whose JIT endpoint failed to build); "
            "ERROR_INSUFFICIENT_BUFFER: more than kMaxCommitActions operations in one transaction; "
            "ERROR_INVALID_ADDRESS / ERROR_NOT_SUPPORTED: MinHook can't hook the target (not executable, or a "
            "prologue it can't relocate)");
        if (status_ == NO_ERROR) { status_ = status; }
    }

    [[nodiscard]]
    DWORD remainingMs() const {
        const ULONGLONG now = GetTickCount64();
        return now >= deadline_ ? 0 : static_cast<DWORD>(deadline_ - now);
    }

    LONG acquire() {
        if (!detail::transaction_mutex->try_lock_for(std::chrono::milliseconds(remainingMs()))) {
            return ERROR_TIMEOUT;
        }
        lock_held_ = true;
        detail::transaction_owner->store(GetCurrentThreadId(), std::memory_order_release);
        detail::transaction_joined->store(false, std::memory_order_relaxed);

        const LONG init = detail::mhLazyInitialize();
        if (init != NO_ERROR) {
            releaseTransactionLock();
            return init;
        }
        started_ = true;
        detail::transaction_pending->store(true, std::memory_order_release);
        return NO_ERROR;
    }

    LONG acquireWithRetry() {
        for (DWORD backoff_ms = 1;; backoff_ms = (std::min)(backoff_ms * 2, kMaxBackoffMs)) {
            const LONG status = acquire();
            if (status == NO_ERROR || !detail::isTransientStatus(status) || remainingMs() == 0) { return status; }
            Sleep((std::min)(backoff_ms, remainingMs()));
        }
    }

    bool record(const RecordedOp& op) {
        if (!owns_transaction_) {
            detail::transaction_joined->store(true, std::memory_order_relaxed);
            return true;
        }
        if (op_count_ >= kMaxCommitActions) {
            fail(ERROR_INSUFFICIENT_BUFFER);
            return false;
        }
        ops_[op_count_++] = op;
        return true;
    }

    void replayOps() {
        for (std::size_t i = 0; i < op_count_ && status_ == NO_ERROR; ++i) {
            ops_[i].run(*this, ops_[i]);
        }
    }

    void rollbackAttempt() {
        restorePatches();
        if (started_) { detail::runMhAbort(); }
        runAbortActions();
        commit_action_count_ = 0;
        abort_action_count_ = 0;
        pending_touched_count_ = 0;
        if (lock_held_) { joined_ = joined_ || detail::transaction_joined->load(std::memory_order_relaxed); }
        releaseTransactionLock();
        started_ = false;
    }

    LONG applyQueued() {
        std::vector<void*> guarded_targets;
        {
            std::lock_guard lk(*detail::mh_pending_ops_mutex);
            guarded_targets.reserve(detail::mh_pending_ops->size() + patches_.size() * 2);
            for (const auto& op : *detail::mh_pending_ops) {
                guarded_targets.push_back(op.addr);
            }
        }
        for (const auto& patch : patches_) {
            const auto first = reinterpret_cast<std::uintptr_t>(patch.address);
            const std::uintptr_t last = first + patch.bytes.size() - 1;
            guarded_targets.push_back(patch.address);
            for (std::uintptr_t page = (first & detail::kPageGuardPageMask) + detail::kPageGuardPageSize; page <= last;
                page += detail::kPageGuardPageSize) {
                guarded_targets.push_back(reinterpret_cast<void*>(page));
            }
            guarded_targets.push_back(reinterpret_cast<void*>(last));
        }

        LONG status = HOOKKIT_COMMIT_FAULT(detail::CommitStage::eBeforeApply);
        {
            detail::PageGuardSession guard(guarded_targets);
            if (status == NO_ERROR) { status = detail::mapMinHookStatus(MH_ApplyQueued()); }
            if (status == NO_ERROR) { status = HOOKKIT_COMMIT_FAULT(detail::CommitStage::eAfterHooks); }
            if (status == NO_ERROR) { status = applyPatches(); }
            if (status == NO_ERROR) { status = HOOKKIT_COMMIT_FAULT(detail::CommitStage::eAfterPatches); }
        }
        return status;
    }

    LONG applyPatches() {
        for (auto& [address, bytes, backup, applied] : patches_) {
            if (!writeBytes(address, bytes, &backup)) {
                restorePatches();
                return ERROR_NOACCESS;
            }
            applied = true;
        }
        return NO_ERROR;
    }

    void restorePatches() {
        for (auto it = patches_.rbegin(); it != patches_.rend(); ++it) {
            if (!it->applied) { continue; }
            const bool restored = writeBytes(it->address, it->backup, nullptr);
            HOOKKIT_ASSERT(restored,
                "hookkit: could not restore patched bytes -- VirtualProtect refused the page, the patch stays applied");
            it->applied = false;
        }
    }

    static bool writeBytes(void* address, const std::vector<std::uint8_t>& bytes, std::vector<std::uint8_t>* backup) {
        ScopedUnprotect unprotect(address, bytes.size());
        if (!unprotect.ok()) { return false; }
        if (backup != nullptr) {
            const auto* current = static_cast<const std::uint8_t*>(address);
            backup->assign(current, current + bytes.size());
        }
        std::memcpy(address, bytes.data(), bytes.size());
        FlushInstructionCache(GetCurrentProcess(), address, bytes.size());
        return true;
    }

    bool isTouched(void* key) const {
        for (std::size_t i = 0; i < pending_touched_count_; ++i) {
            if (pending_touched_[i] == key) { return true; }
        }
        return false;
    }

    template <typename HookType>
    void attachOne() {
        const RecordedOp op{.run = [](HookTransaction& tx, const RecordedOp&) { tx.doAttach<HookType>(); }};
        if (record(op)) { doAttach<HookType>(); }
    }

    template <typename HookType>
    void detachOne() {
        const RecordedOp op{.run = [](HookTransaction& tx, const RecordedOp&) { tx.doDetach<HookType>(); }};
        if (record(op)) { doDetach<HookType>(); }
    }

    template <typename HookType>
    void doAttach() {
        if (status_ != NO_ERROR) { return; }
        if (!detail::transaction_pending->load(std::memory_order_acquire)) {
            fail(ERROR_INVALID_OPERATION);
            return;
        }
        auto* const key = static_cast<void*>(&HookType::attached);
        if (isTouched(key) || HookType::attached.load(std::memory_order_acquire)) {
            fail(ERROR_ALREADY_EXISTS);
            return;
        }
        void* const detour = HookType::resolveDetour();
        if (!detour) {
            fail(ERROR_INVALID_FUNCTION);
            return;
        }

        PVOID* const slot = HookType::patchSlot();
        void* const target_addr = *slot;
        LPVOID trampoline = nullptr;
        MH_STATUS mh = MH_CreateHook(target_addr, detour, &trampoline);
        if (mh != MH_OK) {
            fail(detail::mapMinHookStatus(mh));
            return;
        }
        *slot = trampoline;
        {
            std::lock_guard lk(*detail::mh_map_mutex);
            (*detail::mh_original_by_trampoline)[trampoline] = target_addr;
            (*detail::ip_boundary_by_target)[target_addr] = detail::buildIpBoundaryInfo(target_addr, trampoline);
        }
        detail::pushMhPendingOp({target_addr, trampoline, slot, /*is_detach=*/false, +[]() {
            detail::flushQuiesce();
            const ULONGLONG deadline = GetTickCount64() + kDetachDrainTimeoutMs;
            while (!HookType::quiescent()) {
                if (GetTickCount64() >= deadline) { break; }
                YieldProcessor();
            }
        }});

        mh = MH_QueueEnableHook(target_addr);
        if (mh != MH_OK) {
            fail(detail::mapMinHookStatus(mh));
            return;
        }

        if (pending_touched_count_ >= kMaxCommitActions) {
            fail(ERROR_INSUFFICIENT_BUFFER);
            return;
        }
        pending_touched_[pending_touched_count_++] = key;
        pushCommitAction([] { HookType::attached.store(true, std::memory_order_release); });
    }

    template <typename HookType>
    void doDetach() {
        if (status_ != NO_ERROR) { return; }
        if (!detail::transaction_pending->load(std::memory_order_acquire)) {
            fail(ERROR_INVALID_OPERATION);
            return;
        }
        auto* const key = static_cast<void*>(&HookType::attached);
        if (isTouched(key)) {
            fail(ERROR_NOT_FOUND);
            return;
        }
        if (!HookType::attached.load(std::memory_order_acquire)) {
            fail(ERROR_NOT_FOUND);
            return;
        }

        bool exclude_self = false;
        {
            std::lock_guard lk(*detail::mh_map_mutex);
            const auto orig_it = detail::mh_original_by_trampoline->find(*HookType::patchSlot());
            if (orig_it != detail::mh_original_by_trampoline->end()) {
                const auto info_it = detail::ip_boundary_by_target->find(orig_it->second);
                exclude_self = info_it != detail::ip_boundary_by_target->end() && !info_it->second.prologue_has_call;
            }
        }

        HookType::detaching.store(true, std::memory_order_seq_cst);

        detail::flushQuiesce();
        if constexpr (kDetachDrainTimeoutMs == 0) {
            while (!HookType::quiescent(exclude_self)) {
                YieldProcessor();
            }
        } else {
            const ULONGLONG deadline = GetTickCount64() + kDetachDrainTimeoutMs;
            while (!HookType::quiescent(exclude_self)) {
                if (GetTickCount64() >= deadline) {
                    HookType::detaching.store(false, std::memory_order_seq_cst);
                    fail(ERROR_TIMEOUT);
                    return;
                }
                YieldProcessor();
            }
        }

        PVOID* const slot = HookType::patchSlot();
        void* const trampoline = *slot;
        void* original;
        {
            std::lock_guard lk(*detail::mh_map_mutex);
            auto& map = *detail::mh_original_by_trampoline;
            auto it = map.find(trampoline);
            if (it == map.end()) {
                HookType::detaching.store(false, std::memory_order_seq_cst);
                fail(ERROR_NOT_FOUND);
                return;
            }
            original = it->second;
        }

        const LONG queued = detail::mapMinHookStatus(MH_QueueDisableHook(original));
        if (queued != NO_ERROR) {
            HookType::detaching.store(false, std::memory_order_seq_cst);
            fail(queued);
            return;
        }
        *slot = original;
        detail::pushMhPendingOp({.addr = original, .trampoline = trampoline, .slot = slot, .is_detach = true});

        if (pending_touched_count_ >= kMaxCommitActions) {
            HookType::detaching.store(false, std::memory_order_seq_cst);
            fail(ERROR_INSUFFICIENT_BUFFER);
            return;
        }
        pending_touched_[pending_touched_count_++] = key;
        pushCommitAction([] {
            HookType::attached.store(false, std::memory_order_release);
            HookType::hook.store(nullptr, std::memory_order_release);
            HookType::detaching.store(false, std::memory_order_seq_cst);
        });
        pushAbortAction([] { HookType::detaching.store(false, std::memory_order_seq_cst); });
        if (status_ != NO_ERROR) { HookType::detaching.store(false, std::memory_order_seq_cst); }
    }

    void doAttachRaw(void** slot, void* detour) {
        if (status_ != NO_ERROR) { return; }
        if (!detail::transaction_pending->load(std::memory_order_acquire)) {
            fail(ERROR_INVALID_OPERATION);
            return;
        }
        void* const target_addr = *slot;
        LPVOID trampoline = nullptr;
        MH_STATUS mh = MH_CreateHook(target_addr, detour, &trampoline);
        if (mh != MH_OK) {
            fail(detail::mapMinHookStatus(mh));
            return;
        }
        {
            std::lock_guard lk(*detail::mh_map_mutex);
            (*detail::mh_original_by_trampoline)[trampoline] = target_addr;
            (*detail::ip_boundary_by_target)[target_addr] = detail::buildIpBoundaryInfo(target_addr, trampoline);
        }
        detail::pushMhPendingOp({.addr = target_addr, .trampoline = trampoline, .slot = slot, .is_detach = false});

        mh = MH_QueueEnableHook(target_addr);
        if (mh != MH_OK) {
            fail(detail::mapMinHookStatus(mh));
            return;
        }
        *slot = trampoline;
    }

    void doDetachRaw(void** slot) {
        if (status_ != NO_ERROR) { return; }
        if (!detail::transaction_pending->load(std::memory_order_acquire)) {
            fail(ERROR_INVALID_OPERATION);
            return;
        }
        void* const trampoline = *slot;
        void* original;
        {
            std::lock_guard lk(*detail::mh_map_mutex);
            auto& map = *detail::mh_original_by_trampoline;
            auto it = map.find(trampoline);
            if (it == map.end()) {
                fail(ERROR_NOT_FOUND);
                return;
            }
            original = it->second;
        }
        MH_STATUS mh = MH_QueueDisableHook(original);
        if (mh != MH_OK) {
            fail(detail::mapMinHookStatus(mh));
            return;
        }
        detail::pushMhPendingOp({.addr = original, .trampoline = trampoline, .slot = slot, .is_detach = true});
        *slot = original;
    }

    void pushCommitAction(void (*action)()) {
        if (commit_action_count_ >= kMaxCommitActions) {
            fail(ERROR_INSUFFICIENT_BUFFER);
            return;
        }
        commit_actions_[commit_action_count_++] = action;
    }

    void runCommitActions() const {
        for (std::size_t i = 0; i < commit_action_count_; ++i) {
            commit_actions_[i]();
        }
    }

    void pushAbortAction(void (*action)()) {
        if (abort_action_count_ >= kMaxCommitActions) {
            fail(ERROR_INSUFFICIENT_BUFFER);
            return;
        }
        abort_actions_[abort_action_count_++] = action;
    }

    void runAbortActions() const {
        for (std::size_t i = 0; i < abort_action_count_; ++i) {
            abort_actions_[i]();
        }
    }

    void releaseTransactionLock() {
        if (lock_held_) {
            detail::transaction_pending->store(false, std::memory_order_release);
            detail::transaction_owner->store(0, std::memory_order_release);
            detail::transaction_mutex->unlock();
            lock_held_ = false;
        }
    }

    static constexpr std::size_t kMaxCommitActions = 64;

    static constexpr ULONGLONG kDetachDrainTimeoutMs = HOOKKIT_DETACH_DRAIN_TIMEOUT_MS;

    static constexpr DWORD kMaxBackoffMs = 16;

    bool owns_transaction_ = true;
    bool suspend_all_threads_ = true;
    bool started_ = false;
    bool committed_ = false;
    bool joined_ = false;
    LONG status_ = NO_ERROR;
    bool lock_held_ = false;
    ULONGLONG deadline_ = 0;
    std::array<void (*)(), kMaxCommitActions> commit_actions_{};
    std::size_t commit_action_count_ = 0;
    std::array<void (*)(), kMaxCommitActions> abort_actions_{};
    std::size_t abort_action_count_ = 0;
    std::array<void*, kMaxCommitActions> pending_touched_{};
    std::size_t pending_touched_count_ = 0;
    std::array<RecordedOp, kMaxCommitActions> ops_{};
    std::size_t op_count_ = 0;
    std::vector<PendingPatch> patches_;
};

inline void shutdown() { detail::removePageGuardVeh(); }

}  // namespace hookkit
