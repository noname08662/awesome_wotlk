#pragma once

#include <hookkit/hookkit.h>
#include <windows.h>

#include <cstdint>
#include <memory>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

#include "BaseTypes.h"

namespace storm {
namespace memory {

// flags, change_mask
HOOKKIT_HOOK_HANDLE(setDebugFlags, 0x0076E4A0, hookkit::Conv::eStdcall, void, uint32_t, uint32_t);
// bytes, file_name, line_number, flags
HOOKKIT_HOOK_HANDLE(alloc, 0x0076E540, hookkit::Conv::eStdcall, void*, uint32_t, const char*, int32_t, uint32_t);
// ptr, file_name, line_number, flags
HOOKKIT_HOOK_HANDLE(free, 0x0076E5A0, hookkit::Conv::eStdcall, int32_t, void*, const char*, int32_t, uint32_t);
// ptr, bytes, file_name, line_number, flags
HOOKKIT_HOOK_HANDLE(
    reAlloc, 0x0076E5E0, hookkit::Conv::eStdcall, void*, void*, uint32_t, const char*, int32_t, uint32_t);
// bytes, file_name, line_number
HOOKKIT_HOOK_HANDLE(alignedAlloc, 0x0083DE50, hookkit::Conv::eCdecl, void*, uint32_t, const char*, int32_t);
// ptr
HOOKKIT_HOOK_HANDLE(alignedFree, 0x0083DE90, hookkit::Conv::eCdecl, void, void*);

}  // namespace memory

namespace file {

// filename, handle
HOOKKIT_HOOK_HANDLE(open, 0x00424F80, hookkit::Conv::eStdcall, int, const char*, void**);
// archive, filename, flags, handle
HOOKKIT_HOOK_HANDLE(openEx, 0x00424B50, hookkit::Conv::eStdcall, int, void*, const char*, uint32_t, void**);
// file_name, buffer, size, extra_size, flags
HOOKKIT_HOOK_HANDLE(loadFile, 0x00424F50, hookkit::Conv::eStdcall, int, const char*, void**, size_t*, size_t, uint32_t);
// buffer
HOOKKIT_HOOK_HANDLE(unloadFile, 0x00422090, hookkit::Conv::eStdcall, int32_t, void*);
// file_handle, file_size_high
HOOKKIT_HOOK_HANDLE(getFileSize, 0x004218C0, hookkit::Conv::eStdcall, uint32_t, void*, uint32_t*);
// file_handle, distance_to_move, distance_to_move_high, move_method
HOOKKIT_HOOK_HANDLE(setFilePointer, 0x00421BB0, hookkit::Conv::eStdcall, uint32_t, void*, int32_t, int32_t*, uint32_t);
// file_handle, buffer, number_of_bytes_to_read, number_of_bytes_read, overlapped, async_action
HOOKKIT_HOOK_HANDLE(read, 0x00422530, hookkit::Conv::eStdcall, bool, void*, void*, size_t, size_t*, void*, void*);
// file_handle
HOOKKIT_HOOK_HANDLE(close, 0x00422910, hookkit::Conv::eStdcall, bool, void*);
// filename, flags
HOOKKIT_HOOK_HANDLE(fileExistsEx, 0x00424B10, hookkit::Conv::eStdcall, bool, const char*, uint32_t);
// buffer, buffer_size
HOOKKIT_HOOK_HANDLE(getBasePath, 0x00421880, hookkit::Conv::eStdcall, bool, char*, size_t);
// path
HOOKKIT_HOOK_HANDLE(setBasePath, 0x00421A80, hookkit::Conv::eStdcall, bool, const char*);
// path
HOOKKIT_HOOK_HANDLE(setDataPath, 0x00421AF0, hookkit::Conv::eStdcall, bool, const char*);
// buffer
HOOKKIT_HOOK_HANDLE(unload, 0x00421CA0, hookkit::Conv::eStdcall, bool, void*);
HOOKKIT_HOOK_HANDLE(setStreamingStatus, 0x004220B0, hookkit::Conv::eCdecl, int, char, char, char, char, int);
// lpsz_url
HOOKKIT_HOOK_HANDLE(initializeStreaming, 0x00422100, hookkit::Conv::eStdcall, int, const char*);
HOOKKIT_HOOK_HANDLE(isStreamingMode, 0x00422130, hookkit::Conv::eCdecl, bool);
HOOKKIT_HOOK_HANDLE(getErrorDetails, 0x00422150, hookkit::Conv::eCdecl, char*);
// archive, filename, buffer, size, extra_size, flags, async_action
HOOKKIT_HOOK_HANDLE(
    load, 0x00424E80, hookkit::Conv::eStdcall, bool, void*, const char*, void**, size_t*, size_t, uint32_t, void*);
HOOKKIT_HOOK_HANDLE(disableSFileCheckDisk, 0x00421750, hookkit::Conv::eCdecl, void);
// flags
HOOKKIT_HOOK_HANDLE(enableDirectAccess, 0x00421760, hookkit::Conv::eStdcall, bool, uint32_t);
// file_handle
HOOKKIT_HOOK_HANDLE(fileIsLocal, 0x004217C0, hookkit::Conv::eStdcall, bool, void*);
// file_handle, priority, a3
HOOKKIT_HOOK_HANDLE(boostPriority, 0x00421820, hookkit::Conv::eStdcall, bool, void*, int, char);
// archive, filename, buffer, size, extra_size, flags, async_action
HOOKKIT_HOOK_HANDLE(loadFileEx, 0x00421FF0, hookkit::Conv::eStdcall, bool, void*, const char*, void**, size_t*, size_t,
    uint32_t, void*);
// filename, priority, flags, handle
HOOKKIT_HOOK_HANDLE(openArchive, 0x00422040, hookkit::Conv::eStdcall, bool, const char*, uint32_t, uint32_t, void**);
HOOKKIT_HOOK_HANDLE(rebuildHash, 0x00423D70, hookkit::Conv::eCdecl, void);

inline constexpr hookkit::WildAbi<1> kGetFileExtensionAbi = {{hookkit::ArgLoc::inReg(hookkit::Reg::eSi)}};

// src
HOOKKIT_HOOK_WILD_HANDLE(
    getFileExtension, 0x004B51C0, hookkit::Conv::eUsercall, char*, kGetFileExtensionAbi, const char*);

}  // namespace file

namespace str {

// string, c
HOOKKIT_HOOK_HANDLE(chr, 0x0076E6E0, hookkit::Conv::eCdecl, const char*, const char*, char);
// string, c
HOOKKIT_HOOK_HANDLE(chrR, 0x0076E720, hookkit::Conv::eCdecl, const char*, const char*, char);
// string1, string2, size
HOOKKIT_HOOK_HANDLE(cmp, 0x0076E760, hookkit::Conv::eStdcall, int, const char*, const char*, size_t);
// string1, string2, size
HOOKKIT_HOOK_HANDLE(cmpI, 0x0076E780, hookkit::Conv::eStdcall, int, const char*, const char*, size_t);
// dest, src, max_chars
HOOKKIT_HOOK_HANDLE(copy, 0x0076ED20, hookkit::Conv::eStdcall, int, char*, const char*, size_t);
// str
HOOKKIT_HOOK_HANDLE(len, 0x0076EE30, hookkit::Conv::eStdcall, int, const char*);
// dest, src, max_chars
HOOKKIT_HOOK_HANDLE(pack, 0x0076EF70, hookkit::Conv::eStdcall, int, char*, const char*, size_t);
// dest, max_chars, format
HOOKKIT_HOOK_HANDLE(printf, 0x0076F070, hookkit::Conv::eCdecl, int, char*, size_t, const char*);
// dest, max_chars, format, args
HOOKKIT_HOOK_HANDLE(vPrintf, 0x0076F0A0, hookkit::Conv::eCdecl, int, char*, size_t, const char*, va_list);
// string
HOOKKIT_HOOK_HANDLE(toInt, 0x0076F0D0, hookkit::Conv::eStdcall, int, const char*);
// string
HOOKKIT_HOOK_HANDLE(toUnsigned, 0x0076F140, hookkit::Conv::eStdcall, uint32_t, const char*);
// string
HOOKKIT_HOOK_HANDLE(toUnsignedPtr, 0x0076F190, hookkit::Conv::eStdcall, uint32_t, const char**);
// string, buffer, buffer_size, delimiters, out_flag
HOOKKIT_HOOK_HANDLE(
    tokenize, 0x0076F1E0, hookkit::Conv::eStdcall, void, const char**, char*, size_t, const char*, int*);
// string, flags, hash_seed
HOOKKIT_HOOK_HANDLE(hash, 0x0076F340, hookkit::Conv::eStdcall, int, const char*, uint32_t, int);
// string
HOOKKIT_HOOK_HANDLE(hashUtf8, 0x0076F640, hookkit::Conv::eStdcall, uint32_t, const char*);
// string
HOOKKIT_HOOK_HANDLE(upper, 0x0076F6C0, hookkit::Conv::eStdcall, char*, char*);
// string
HOOKKIT_HOOK_HANDLE(lower, 0x0076F6E0, hookkit::Conv::eStdcall, char*, char*);
// string, search
HOOKKIT_HOOK_HANDLE(str, 0x0076F700, hookkit::Conv::eCdecl, const char*, const char*, const char*);
// string, search
HOOKKIT_HOOK_HANDLE(strI, 0x0076F770, hookkit::Conv::eCdecl, const char*, const char*, const char*);
// string, search
HOOKKIT_HOOK_HANDLE(strUtf8I, 0x0076F7E0, hookkit::Conv::eCdecl, const char*, const char*, const char*);
// value, dest, size
HOOKKIT_HOOK_HANDLE(formatInt64, 0x0076F860, hookkit::Conv::eCdecl, char*, __int64, char*, size_t);
// string, file_name, line_number
HOOKKIT_HOOK_HANDLE(dupA, 0x0076F9E0, hookkit::Conv::eStdcall, char*, const char*, const char*, int32_t);
// string
HOOKKIT_HOOK_HANDLE(toUInt64, 0x0076FA50, hookkit::Conv::eStdcall, uint64_t, const char*);
// string
HOOKKIT_HOOK_HANDLE(toFloat, 0x0076FB80, hookkit::Conv::eStdcall, double, const char*);
// utf8_out, utf8_out_size, utf16_in, utf16_in_len, out_bytes_used, out_chars_consumed
HOOKKIT_HOOK_HANDLE(
    convertUtf16to8, 0x00775BD0, hookkit::Conv::eStdcall, int, char*, int, const uint16_t*, int, uint32_t*, int*);
// utf8_in, utf8_in_len, out_bytes_used
HOOKKIT_HOOK_HANDLE(convertUtf8to16Len, 0x00775D90, hookkit::Conv::eStdcall, int, const char*, int, uint32_t*);
// utf16_out, utf16_out_size, utf8_in, utf8_in_len, out_chars_written, out_bytes_used
HOOKKIT_HOOK_HANDLE(
    convertUtf8to16, 0x00775EB0, hookkit::Conv::eStdcall, int, uint16_t*, int, const char*, int, int*, uint32_t*);
// dest, src, dest_size, max_chars
HOOKKIT_HOOK_HANDLE(copyUtf8, 0x0076EDA0, hookkit::Conv::eStdcall, int, char*, const char*, size_t, int);
// string
HOOKKIT_HOOK_HANDLE(lenW, 0x0076EE60, hookkit::Conv::eStdcall, int, const wchar_t*);
// string
HOOKKIT_HOOK_HANDLE(lenUtf8, 0x0076EEA0, hookkit::Conv::eStdcall, int, const char*);
// string, max_bytes
HOOKKIT_HOOK_HANDLE(lenUtf8Ex, 0x0076EEE0, hookkit::Conv::eStdcall, int, const char*, int);
// string, num_chars
HOOKKIT_HOOK_HANDLE(skipUtf8Chars, 0x0076EF30, hookkit::Conv::eStdcall, const char*, const char*, int);
// string1, string2, size
HOOKKIT_HOOK_HANDLE(cmpUtf8I, 0x0076EA40, hookkit::Conv::eStdcall, int, const char*, const char*, size_t);
// string1, string2, size
HOOKKIT_HOOK_HANDLE(cmpUtf8, 0x0076EC80, hookkit::Conv::eStdcall, int, const char*, const char*, size_t);
// dest, max_chars, format, a4
HOOKKIT_HOOK_HANDLE(vsnPrintf, 0x00773E60, hookkit::Conv::eCdecl, char*, char*, int, char*, int);

}  // namespace str

namespace err {

HOOKKIT_HOOK_HANDLE(getLastError, 0x007717E0, hookkit::Conv::eCdecl, int);
HOOKKIT_HOOK_HANDLE(getLastErrorInternal, 0x007717F0, hookkit::Conv::eCdecl, int);
HOOKKIT_HOOK_HANDLE(setLastErrorInternal, 0x00771800, hookkit::Conv::eStdcall, void, int, int);
// dw_err_code
HOOKKIT_HOOK_HANDLE(setLastError, 0x00771870, hookkit::Conv::eStdcall, void, int);
// src
HOOKKIT_HOOK_HANDLE(setLogTitleString, 0x00771890, hookkit::Conv::eStdcall, void, char*);
HOOKKIT_HOOK_HANDLE(
    setLogTitleCallback, 0x00771900, hookkit::Conv::eStdcall, void, int(__stdcall*)(uint32_t, uint32_t));
// dest, num
HOOKKIT_HOOK_HANDLE(getErrorStrInternal1, 0x00771960, hookkit::Conv::eStdcall, int, char*, int);
// dw_message_id, lp_buffer, n_size
HOOKKIT_HOOK_HANDLE(getErrorStr, 0x00771A80, hookkit::Conv::eStdcall, bool, uint32_t, char*, uint32_t);
HOOKKIT_HOOK_HANDLE(registerHandler, 0x00771B80, hookkit::Conv::eStdcall, void, int);
// dw_message_id, ?, exit_code, ?, ?, u_exit_code, ?
HOOKKIT_HOOK_HANDLE(
    displayError, 0x00771D10, hookkit::Conv::eStdcall, int, uint32_t, int, signed int, char*, int, UINT, int);
// dw_message_id, ?, ?
HOOKKIT_HOOK_HANDLE(catchUnhandledExceptionsInternal4, 0x00772A80, hookkit::Conv::eCdecl, int, uint32_t, char*, char);
// ?, arg0
HOOKKIT_HOOK_HANDLE(catchUnhandledExceptionsInternal6, 0x00772AA0, hookkit::Conv::eCdecl, void, int, char*);
// dw_message_id, ?, exit_code, ?, u_exit_code, ?, ?
HOOKKIT_HOOK_HANDLE(catchUnhandledExceptionsInternal8, 0x00772AC0, hookkit::Conv::eCdecl, int, uint32_t, int, uint32_t,
    int, UINT, char*, char);
HOOKKIT_HOOK_HANDLE(catchUnhandledExceptions, 0x00772B20, hookkit::Conv::eCdecl, int);

}  // namespace err

namespace cmd {

// hash
HOOKKIT_HOOK_HANDLE(getNum, 0x00773460, hookkit::Conv::eStdcall, int32_t, uint32_t);
// , dest, num
HOOKKIT_HOOK_HANDLE(registerArgListInternal1, 0x007734C0, hookkit::Conv::eStdcall, int, int, char*, int);
// cmd_array, num_cmds
HOOKKIT_HOOK_HANDLE(registerArgList, 0x00773590, hookkit::Conv::eStdcall, int32_t, void**, uint32_t);
// hash
HOOKKIT_HOOK_HANDLE(getBool, 0x00773870, hookkit::Conv::eStdcall, uint32_t, uint32_t);
// cmd_line, a2, a3, a4
HOOKKIT_HOOK_HANDLE(process, 0x00773890, hookkit::Conv::eStdcall, bool, char*, int, int, int);
// a1, a2
HOOKKIT_HOOK_HANDLE(processCommandLine, 0x00773990, hookkit::Conv::eStdcall, bool, int, int);

}  // namespace cmd

namespace sync {

// target
HOOKKIT_HOOK_HANDLE(interlockedIncrement, 0x0076FEA0, hookkit::Conv::eFastcall, int, volatile LONG*);
// target
HOOKKIT_HOOK_HANDLE(interlockedDecrement, 0x0076FEF0, hookkit::Conv::eFastcall, int, volatile LONG*);

namespace thread {

// ?, ?, lp_thread_id, ?, src, dw_stack_size
HOOKKIT_HOOK_HANDLE(create2, 0x00770260, hookkit::Conv::eStdcall, HANDLE, int, int, uint32_t*, int, char*, SIZE_T);
HOOKKIT_HOOK_HANDLE(wait, 0x00770290, hookkit::Conv::eStdcall, int, unsigned int, unsigned int, int, int);
// creation_flags, thread_proc, ph_thread, thread_param, dw_stack_size
HOOKKIT_HOOK_HANDLE(create, 0x00774740, hookkit::Conv::eCdecl, bool, int, void*, void**, void*, SIZE_T);
HOOKKIT_HOOK_HANDLE(getCurrentId, 0x0076FDB0, hookkit::Conv::eStdcall, uint32_t);
HOOKKIT_HOOK_HANDLE(getCurrentPriority, 0x0076FDD0, hookkit::Conv::eCdecl, int);
// n_priority
HOOKKIT_HOOK_HANDLE(setCurrentPriority, 0x0076FDE0, hookkit::Conv::eStdcall, int32_t, int);
HOOKKIT_HOOK_HANDLE(setPriority, 0x00774770, hookkit::Conv::eThiscall, bool, HANDLE*, int);

}  // namespace thread

namespace crit_sect {

// lp_critical_section
HOOKKIT_HOOK_HANDLE(lock, 0x00770FF0, hookkit::Conv::eCdecl, void, LPCRITICAL_SECTION);
// lp_critical_section
HOOKKIT_HOOK_HANDLE(constructor, 0x00774620, hookkit::Conv::eFastcall, LPCRITICAL_SECTION, LPCRITICAL_SECTION);
// lp_critical_section
HOOKKIT_HOOK_HANDLE(destructor, 0x00774630, hookkit::Conv::eFastcall, void, LPCRITICAL_SECTION);
// lp_critical_section
HOOKKIT_HOOK_HANDLE(enter, 0x00774640, hookkit::Conv::eFastcall, void, LPCRITICAL_SECTION);
// lp_critical_section
HOOKKIT_HOOK_HANDLE(leave, 0x00774650, hookkit::Conv::eFastcall, void, LPCRITICAL_SECTION);
HOOKKIT_HOOK_HANDLE(iteratorBaseHasContainer, 0x00774680, hookkit::Conv::eCdecl, void*);

}  // namespace crit_sect

namespace event {

HOOKKIT_HOOK_HANDLE(set, 0x00774720, hookkit::Conv::eThiscall, bool, HANDLE*);
HOOKKIT_HOOK_HANDLE(reset, 0x00774730, hookkit::Conv::eThiscall, int32_t, HANDLE*);
// b_manual_reset, b_initial_state
HOOKKIT_HOOK_HANDLE(constructor, 0x00774900, hookkit::Conv::eThiscall, uint32_t*, uint32_t*, int32_t, int32_t);

}  // namespace event

namespace mutex {

// b_initial_owner, lp_name
HOOKKIT_HOOK_HANDLE(create, 0x007747C0, hookkit::Conv::eThiscall, void, HANDLE*, int32_t, const char*);
HOOKKIT_HOOK_HANDLE(release, 0x00774810, hookkit::Conv::eThiscall, int32_t, HANDLE*);
HOOKKIT_HOOK_HANDLE(releaseInternal1, 0x00774820, hookkit::Conv::eCdecl, void*);
HOOKKIT_HOOK_HANDLE(releaseInternal2, 0x00774870, hookkit::Conv::eThiscall, int, uint32_t);
HOOKKIT_HOOK_HANDLE(createEx, 0x00774970, hookkit::Conv::eThiscall, uint32_t*, uint32_t*);

}  // namespace mutex

namespace semaphore {

// l_release_count
HOOKKIT_HOOK_HANDLE(release, 0x007747A0, hookkit::Conv::eThiscall, int32_t, HANDLE*, LONG);
// l_initial_count, l_maximum_count
HOOKKIT_HOOK_HANDLE(create, 0x00774940, hookkit::Conv::eThiscall, HANDLE*, HANDLE*, LONG, LONG);

}  // namespace semaphore

namespace object {

// dw_milliseconds
HOOKKIT_HOOK_HANDLE(wait, 0x00774690, hookkit::Conv::eThiscall, uint32_t, HANDLE*, uint32_t);
// num_group_members, group_members, b_wait_all, dw_milliseconds
HOOKKIT_HOOK_HANDLE(
    waitForMultiple, 0x007746B0, hookkit::Conv::eStdcall, uint32_t, unsigned int, int, int32_t, uint32_t);
HOOKKIT_HOOK_HANDLE(close, 0x007748E0, hookkit::Conv::eThiscall, HANDLE, HANDLE*);

}  // namespace object
}  // namespace sync

namespace log {

HOOKKIT_HOOK_HANDLE(prepareInitialize, 0x0076E490, hookkit::Conv::eCdecl, void);
// log_handle
HOOKKIT_HOOK_HANDLE(close, 0x007754A0, hookkit::Conv::eStdcall, void, int);
// log_handle
HOOKKIT_HOOK_HANDLE(flush, 0x00775500, hookkit::Conv::eStdcall, void, int);
// src, flags, log_handle
HOOKKIT_HOOK_HANDLE(create, 0x007757E0, hookkit::Conv::eStdcall, int, const char*, uint32_t, int*);
HOOKKIT_HOOK_HANDLE(destroy, 0x007758E0, hookkit::Conv::eCdecl, void);
// log_handle, format, va
HOOKKIT_HOOK_HANDLE(vWrite, 0x00775A90, hookkit::Conv::eStdcall, void, int, const char*, va_list);
// log_handle, format
HOOKKIT_HOOK_HANDLE(write, 0x00775BB0, hookkit::Conv::eCdecl, uint64_t, int, const char*);

}  // namespace log

namespace registry {

// keyname, valuename, flags, buffer, bytes
HOOKKIT_HOOK_HANDLE(
    loadString, 0x00770720, hookkit::Conv::eStdcall, int, const char*, const char*, uint32_t, char*, uint32_t);
// keyname, valuename, flags, value
HOOKKIT_HOOK_HANDLE(
    loadValue, 0x00770840, hookkit::Conv::eStdcall, int32_t, const char*, const char*, uint32_t, uint32_t*);
// keyname, valuename, flags, str
HOOKKIT_HOOK_HANDLE(
    saveString, 0x007708F0, hookkit::Conv::eStdcall, int32_t, const char*, const char*, uint32_t, const char*);
// keyname, valuename, flags, value
HOOKKIT_HOOK_HANDLE(
    saveValue, 0x007709A0, hookkit::Conv::eStdcall, int32_t, const char*, const char*, uint32_t, uint32_t);

}  // namespace registry

namespace process {

HOOKKIT_HOOK_HANDLE(getCurrentId, 0x0076FDC0, hookkit::Conv::eStdcall, uint32_t);
// dw_process_affinity_mask
HOOKKIT_HOOK_HANDLE(setAffinityMask, 0x0076FE00, hookkit::Conv::eStdcall, int32_t, uint32_t*);
// u_exit_code
HOOKKIT_HOOK_HANDLE(exit, 0x00773A40, hookkit::Conv::eCdecl, void, int);

}  // namespace process

namespace signature {

HOOKKIT_HOOK_HANDLE(update, 0x00770CA0, hookkit::Conv::eCdecl, int, uint32_t*, int, int);
HOOKKIT_HOOK_HANDLE(initialize, 0x00770D50, hookkit::Conv::eCdecl, int, int, int, int);
HOOKKIT_HOOK_HANDLE(verifyStream, 0x00770DB0, hookkit::Conv::eCdecl, bool, int*, int, int);

}  // namespace signature

namespace ar_c4 {

HOOKKIT_HOOK_HANDLE(process, 0x00774EA0, hookkit::Conv::eCdecl, uint8_t*, int, unsigned int, uint8_t*, uint8_t*);
HOOKKIT_HOOK_HANDLE(init, 0x00775040, hookkit::Conv::eCdecl, void, int, unsigned int, int);

}  // namespace ar_c4

namespace rgn {

HOOKKIT_HOOK_HANDLE(combineRectf, 0x00777420, hookkit::Conv::eStdcall, void, int, float*, int, int);
HOOKKIT_HOOK_HANDLE(getBoundingRectf, 0x00777590, hookkit::Conv::eStdcall, void, int, float*);
HOOKKIT_HOOK_HANDLE(unlink, 0x00777940, hookkit::Conv::eStdcall, void, int);
// node
HOOKKIT_HOOK_HANDLE(create, 0x00777980, hookkit::Conv::eCdecl, void, void*, int*, int);

}  // namespace rgn

namespace ts {
template <typename T>
struct TSLink {
    TSLink<T>* prev_link;
    T* next;
};

template <typename T>
struct TSGetLink {
    static TSLink<T>& link(T* ptr) { return ptr->link; }

    static const TSLink<T>& link(const T* ptr) { return ptr->link; }

    static T* node(TSLink<T>* lnk) {
        return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(lnk) - offsetof(T, link));
    }
};

template <typename T, typename GetLink>
struct TSStaticAccessor {
    TSLink<T>& link(T* ptr) const { return GetLink::link(ptr); }

    T* node(TSLink<T>* lnk) const { return GetLink::node(lnk); }
};

template <typename T>
struct TSOffsetAccessor {
    uintptr_t offset = 0;

    TSOffsetAccessor() = default;

    explicit TSOffsetAccessor(uintptr_t link_offset) : offset(link_offset) {}

    TSLink<T>& link(T* ptr) const { return *reinterpret_cast<TSLink<T>*>(reinterpret_cast<uintptr_t>(ptr) + offset); }

    T* node(TSLink<T>* lnk) const { return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(lnk) - offset); }
};

template <typename T, typename Accessor>
struct TSListImpl {
    Accessor accessor;
    TSLink<T> link;

    TSListImpl() { unlinkAll(); }

    explicit TSListImpl(Accessor acc) : accessor(std::move(acc)) { unlinkAll(); }

    static T* purify(T* ptr) { return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) & ~uintptr_t(1)); }

    static bool isTerminal(const T* ptr) { return (reinterpret_cast<uintptr_t>(ptr) & 1) != 0; }

    T* head() const {
        T* ptr = link.next;
        if (!ptr || isTerminal(ptr)) { return nullptr; }
        return ptr;
    }

    T* tail() const {
        if (link.prev_link == &link) { return nullptr; }
        return accessor.node(link.prev_link);
    }

    T* next(T* ptr) const {
        T* next_ptr = accessor.link(ptr).next;
        if (!next_ptr || isTerminal(next_ptr)) { return nullptr; }
        return next_ptr;
    }

    T* prev(T* ptr) const {
        TSLink<T>* prev_link = accessor.link(ptr).prev_link;
        if (prev_link == &link) { return nullptr; }
        return accessor.node(prev_link);
    }

    void linkToTail(T* ptr) {
        TSLink<T>& node_link = accessor.link(ptr);
        TSLink<T>* old_tail = link.prev_link;

        node_link.prev_link = old_tail;
        node_link.next = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(&link) | 1);

        link.prev_link = &node_link;
        if (old_tail == &link) {
            link.next = ptr;
        } else {
            old_tail->next = ptr;
        }
    }

    void linkToHead(T* ptr) {
        TSLink<T>& node_link = accessor.link(ptr);
        T* old_head = link.next;

        node_link.prev_link = &link;
        node_link.next = old_head;

        if (isTerminal(old_head)) {
            link.prev_link = &node_link;
        } else {
            accessor.link(old_head).prev_link = &node_link;
        }
        link.next = ptr;
    }

    void unlinkNode(T* ptr) {
        TSLink<T>& node_link = accessor.link(ptr);
        TSLink<T>* prev_node = node_link.prev_link;
        T* next_node = node_link.next;

        if (isTerminal(next_node)) {
            link.prev_link = prev_node;
        } else {
            accessor.link(next_node).prev_link = prev_node;
        }

        prev_node->next = next_node;
    }

    void unlinkAll() {
        link.prev_link = &link;
        link.next = reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(&link) | 1);
    }

    bool isEmpty() const { return isTerminal(link.next); }

    template <typename Callback>
    bool enumerate(Callback&& callback) const {
        T* curr = head();
        while (curr) {
            T* next_node = next(curr);
            if constexpr (std::is_invocable_r_v<bool, Callback, T*>) {
                if (!callback(curr)) { return false; }
            } else {
                callback(curr);
            }
            curr = next_node;
        }
        return true;
    }

    template <typename Callback>
    bool enumerateReverse(Callback&& callback) const {
        T* curr = tail();
        while (curr) {
            T* prev_node = prev(curr);
            if constexpr (std::is_invocable_r_v<bool, Callback, T*>) {
                if (!callback(curr)) { return false; }
            } else {
                callback(curr);
            }
            curr = prev_node;
        }
        return true;
    }

    struct Iterator {
        const TSListImpl<T, Accessor>* list;
        T* current;

        Iterator(const TSListImpl<T, Accessor>* l, T* node) : list(l), current(node) {}

        T* operator*() const { return current; }

        T* operator->() const { return current; }

        Iterator& operator++() {
            current = list->next(current);
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const { return current == other.current; }

        bool operator!=(const Iterator& other) const { return current != other.current; }
    };

    Iterator begin() const { return Iterator(this, head()); }

    Iterator end() const { return Iterator(this, nullptr); }
};

template <typename T, typename GetLink = TSGetLink<T>>
struct TSList : TSListImpl<T, TSStaticAccessor<T, GetLink>> {
    using TSListImpl<T, TSStaticAccessor<T, GetLink>>::TSListImpl;
};

template <typename T>
struct TSExplicitList : TSListImpl<T, TSOffsetAccessor<T>> {
    TSExplicitList() = default;

    explicit TSExplicitList(uintptr_t link_offset)
        : TSListImpl<T, TSOffsetAccessor<T>>(TSOffsetAccessor<T>(link_offset)) {}
};

namespace detail {
template <typename Elem, typename Callback>
bool enumerateArray(Elem* data, uint32_t count, Callback& callback) {
    if (!data) { return true; }
    const std::span<Elem> items(data, count);
    for (uint32_t i = 0; i < items.size(); ++i) {
        if constexpr (std::is_invocable_r_v<bool, Callback&, Elem&, uint32_t>) {
            if (!callback(items[i], i)) { return false; }
        } else if constexpr (std::is_invocable_v<Callback&, Elem&, uint32_t>) {
            callback(items[i], i);
        } else if constexpr (std::is_invocable_r_v<bool, Callback&, Elem&>) {
            if (!callback(items[i])) { return false; }
        } else {
            callback(items[i]);
        }
    }
    return true;
}
}  // namespace detail

template <typename T>
struct TSBaseArray {
    uint32_t capacity;
    uint32_t count;
    T* data;

    T& operator[](size_t index) { return data[index]; }

    const T& operator[](size_t index) const { return data[index]; }

    uint32_t size() const { return count; }

    bool empty() const { return count == 0; }

    T* begin() { return data; }

    T* end() { return data + count; }

    const T* begin() const { return data; }

    const T* end() const { return data + count; }

    T& front() { return data[0]; }

    const T& front() const { return data[0]; }

    T& back() { return data[count - 1]; }

    const T& back() const { return data[count - 1]; }

    template <typename Callback>
    bool enumerate(Callback&& callback) {
        return detail::enumerateArray(data, count, callback);
    }

    template <typename Callback>
    bool enumerate(Callback&& callback) const {
        return detail::enumerateArray<const T>(data, count, callback);
    }
};

template <typename T>
struct TSGrowableArray : TSBaseArray<T> {
    uint32_t chunk;

    void reserve(uint32_t new_capacity) {
        if (new_capacity > this->capacity) { reallocate(new_capacity); }
    }

    bool setCount(uint32_t new_count) {
        static_assert(std::is_trivially_copyable_v<T>, "setCount neither constructs nor destroys elements");
        if (new_count > this->capacity && !grow(new_count)) { return false; }
        this->count = new_count;
        return true;
    }

    uint32_t resize(uint32_t new_count) {
        uint32_t old_count = this->count;
        setCount(new_count);
        return old_count;
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (this->count < this->capacity) {
            return *::new (static_cast<void*>(this->data + this->count++)) T(std::forward<Args>(args)...);
        }
        T value(std::forward<Args>(args)...);
        if (!grow(this->count + 1)) { throw std::bad_alloc(); }
        return *::new (static_cast<void*>(this->data + this->count++)) T(std::move(value));
    }

    void push_back(const T& value) { emplace_back(value); }

    void push_back(T&& value) { emplace_back(std::move(value)); }

    void pop_back() { std::destroy_at(this->data + --this->count); }

    void clear() {
        std::destroy_n(this->data, this->count);
        this->count = 0;
    }

    void shrink_to_fit() {
        if (this->count < this->capacity) { reallocate(this->count); }
    }

    void swap(TSGrowableArray& other) noexcept { std::swap(*this, other); }

private:
    bool grow(uint32_t min_capacity) {
        uint32_t new_capacity = this->capacity > 0 ? this->capacity : (chunk > 0 ? chunk : 1);
        while (new_capacity < min_capacity) {
            new_capacity = new_capacity > UINT32_MAX / 2 ? min_capacity : new_capacity * 2;
        }
        reallocate(new_capacity);
        return this->capacity >= min_capacity;
    }

    void reallocate(uint32_t new_capacity) {
        static_assert(alignof(T) <= 8, "Storm blocks are 8-byte aligned");
        if (new_capacity == 0) {
            if (this->data) { storm::memory::free{}(this->data, nullptr, 0, 0); }
            this->data = nullptr;
            this->capacity = 0;
            return;
        }
        if (new_capacity > UINT32_MAX / sizeof(T)) { return; }
        const uint32_t bytes = new_capacity * sizeof(T);
        T* new_data;
        if constexpr (std::is_trivially_copyable_v<T>) {
            new_data = static_cast<T*>(this->data ? storm::memory::reAlloc{}(this->data, bytes, nullptr, 0, 0)
                                                  : storm::memory::alloc{}(bytes, nullptr, 0, 0));
            if (!new_data) { return; }
        } else {
            new_data = static_cast<T*>(storm::memory::alloc{}(bytes, nullptr, 0, 0));
            if (!new_data) { return; }
            if (this->data) {
                std::uninitialized_move_n(this->data, this->count, new_data);
                std::destroy_n(this->data, this->count);
                storm::memory::free{}(this->data, nullptr, 0, 0);
            }
        }
        this->data = new_data;
        this->capacity = new_capacity;
    }
};

template <typename T, typename Hashkey = HashKeyStri>
struct TSHashObject {
    uint32_t hash;
    TSLink<T> slot_link;
    TSLink<T> full_link;
    Hashkey key;
};

template <typename T, typename Hashkey = HashKeyStri>
struct TSHashTable {
    struct {
        int32_t(__stdcall* free_node)(T* node);
        T*(__stdcall* alloc_node)(TSHashTable<T, Hashkey>*, size_t extra_bytes, int flags);
        T*(__thiscall* vector_deleting_destructor)(TSHashTable<T, Hashkey>*, char free);
        void(__thiscall* clear)(TSHashTable<T, Hashkey>*);
    }* vmt;

    TSExplicitList<T> fulllist;
    uint32_t fullness_indicator;
    TSGrowableArray<TSExplicitList<T>> slotlistarray;
    uint32_t slotmask;

    TSHashTable() {
        this->vmt = nullptr;
        this->fulllist.offset = offsetof(T, hash_obj.full_link);
        this->fullness_indicator = 0;
        this->slotmask = 0xFFFFFFFF;
    }

    T* get(const char* key) {
        if constexpr (std::is_same_v<Hashkey, const char*> || std::is_same_v<Hashkey, char*>) {
            if (this->slotmask == 0xFFFFFFFF || !key) { return nullptr; }

            uint32_t hash = storm::str::hashUtf8{}(key);
            auto& list = this->slotlistarray[hash & this->slotmask];
            auto* node = list.head();

            while (node) {
                if (node->hash_obj.hash == hash) {
                    if (node->hash_obj.key == key || storm::str::cmpI{}(node->hash_obj.key, key, 0x7FFFFFFF) == 0) {
                        return node;
                    }
                }
                node = list.next(node);
            }
            return nullptr;
        } else {
            return nullptr;
        }
    }

    T* get(uint32_t hash) {
        if (this->slotmask == 0xFFFFFFFF) { return nullptr; }

        uint32_t bucket = hash & this->slotmask;
        auto& list = this->slotlistarray[bucket];
        auto* node = list.head();

        while (node) {
            if (node->hash_obj.hash == hash) { return node; }
            node = list.next(node);
        }
        return nullptr;
    }
};
}  // namespace ts
}  // namespace storm

using storm::ts::TSBaseArray;
using storm::ts::TSExplicitList;
using storm::ts::TSGrowableArray;
using storm::ts::TSHashObject;
using storm::ts::TSHashTable;
using storm::ts::TSLink;
using storm::ts::TSList;
