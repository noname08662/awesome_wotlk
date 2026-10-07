#pragma once

#include <libloaderapi.h>
#include <minwindef.h>
#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <functional>
#include <string>
#include <string_view>

#include "Lib/Storm.h"

struct Win10Api {
    decltype(&::VirtualAlloc2) virtual_alloc2 = nullptr;
    decltype(&::MapViewOfFile3) map_view_of_file3 = nullptr;
    decltype(&::UnmapViewOfFile2) unmap_view_of_file2 = nullptr;
    decltype(&::PrefetchVirtualMemory) prefetch_virtual_memory = nullptr;  // Win8+, optional
};

inline const Win10Api& win10Api() noexcept {
    static const Win10Api api = []() noexcept {
        HMODULE kernel = GetModuleHandleW(L"kernelbase.dll");
        if (!kernel) { return Win10Api{}; }

        return Win10Api{
            .virtual_alloc2 = reinterpret_cast<decltype(&::VirtualAlloc2)>(GetProcAddress(kernel, "VirtualAlloc2")),
            .map_view_of_file3 = reinterpret_cast<decltype(&::MapViewOfFile3)>(GetProcAddress(kernel, "MapViewOfFile3")),
            .unmap_view_of_file2 =
                reinterpret_cast<decltype(&::UnmapViewOfFile2)>(GetProcAddress(kernel, "UnmapViewOfFile2")),
            .prefetch_virtual_memory =
                reinterpret_cast<decltype(&::PrefetchVirtualMemory)>(GetProcAddress(kernel, "PrefetchVirtualMemory")),
        };
    }();
    return api;
}

inline bool isWin10() noexcept {
    const Win10Api& api = win10Api();
    return api.virtual_alloc2 != nullptr && api.map_view_of_file3 != nullptr && api.unmap_view_of_file2 != nullptr;
}

namespace utils {
inline float minPixelSpan(float min_px, float ddc_per_px, float lo, float hi) {
    const float span = hi - lo;
    const float abs_span = std::fabs(span);
    return abs_span >= ddc_per_px * min_px && abs_span < ddc_per_px ? lo + std::copysign(ddc_per_px, span) : hi;
}

inline const char* sessionStamp() {
    static const std::string stamp = [] {
        char s[40] = {};
        SYSTEMTIME t;
        GetLocalTime(&t);
        std::snprintf(
            s, sizeof(s), "%04d-%02d-%02d-%02d.%02d.%02d ", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
        return std::string(s);
    }();
    return stamp.c_str();
}

inline std::string wideToUtf8(std::wstring_view wstr) {
    if (wstr.empty()) { return {}; }
    constexpr size_t kMaxBytesPerUnit = 3;
    std::string result(wstr.size() * kMaxBytesPerUnit, '\0');
    uint32_t bytes_used = 0;
    const int status = storm::str::convertUtf16to8{}(result.data(), static_cast<int>(result.size()),
        reinterpret_cast<const uint16_t*>(wstr.data()), static_cast<int>(wstr.size()), &bytes_used, nullptr);
    if (status > 0) { return {}; }
    result.resize(bytes_used);
    return result;
}

inline std::wstring utf8ToWide(std::string_view utf8_str) {
    if (utf8_str.empty()) { return {}; }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), nullptr, 0);
    if (size_needed <= 0) { return {}; }
    std::wstring result;
    result.resize(size_needed);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.data(), static_cast<int>(utf8_str.size()), result.data(), size_needed);
    return result;
}

inline std::string getFromClipboardU8(HWND hwnd) {
    if (!OpenClipboard(hwnd)) { return {}; }
    HANDLE h_mem = GetClipboardData(CF_UNICODETEXT);
    if (!h_mem) {
        CloseClipboard();
        return {};
    }
    auto* utf16 = static_cast<const wchar_t*>(GlobalLock(h_mem));
    if (!utf16) {
        CloseClipboard();
        return {};
    }
    int utf16_length = std::wcslen(utf16) + 1;
    int utf8_length =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, utf16_length, nullptr, 0, nullptr, nullptr);
    if (utf8_length == 0) {
        GlobalUnlock(h_mem);
        CloseClipboard();
        return {};
    }

    std::string utf8;
    utf8.resize(utf8_length);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, utf16_length, utf8.data(), utf8_length, nullptr, nullptr);

    GlobalUnlock(h_mem);
    CloseClipboard();
    return utf8;
}

inline bool copyToClipboardU8(const char* u8_str, HWND hwnd) {
    if (!u8_str || !u8_str[0]) {
        // just empty
        if (!OpenClipboard(hwnd)) { return false; }
        bool result = EmptyClipboard();
        CloseClipboard();
        return result;
    }
    int u8_chars_len = std::strlen(u8_str) + 1;
    int w_chars_len = MultiByteToWideChar(CP_UTF8, 0, u8_str, u8_chars_len, nullptr, 0);

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * w_chars_len);
    if (!mem) { return false; }

    auto* cb_buf = static_cast<wchar_t*>(GlobalLock(mem));
    if (!cb_buf) {
        GlobalFree(mem);
        return false;
    }

    MultiByteToWideChar(CP_UTF8, 0, u8_str, u8_chars_len, cb_buf, w_chars_len);
    cb_buf[w_chars_len] = L'\0';
    GlobalUnlock(mem);

    if (!OpenClipboard(hwnd)) {
        GlobalFree(mem);
        return false;
    }
    if (!EmptyClipboard()) {
        CloseClipboard();
        GlobalFree(mem);
        return false;
    }

    SetClipboardData(CF_UNICODETEXT, mem);
    CloseClipboard();
    return true;
}

inline bool iequals(std::string_view lhs, std::string_view rhs) {
    return std::ranges::equal(lhs, rhs, [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
    });
}

inline bool iequals(std::wstring_view lhs, std::wstring_view rhs) {
    return std::ranges::equal(lhs, rhs, [](wchar_t a, wchar_t b) {
        return std::towlower(static_cast<wint_t>(a)) == std::towlower(static_cast<wint_t>(b));
    });
}
}  // namespace utils

//constexpr float MAX_TRACE_DISTANCE = 1000.0f;
//constexpr uint32_t TERRAIN_HIT_FLAGS = 0x100171;
//
//using TraceLine_t = uint8_t(__cdecl*)(C3Vector* start, C3Vector* end, C3Vector* hitPoint, float* dist, uint32_t flags, uint32_t opt);
//auto TraceLine_orig = reinterpret_cast<TraceLine_t>(0x007A3B70);
//
//bool TraceLine(const C3Vector& start, const C3Vector& end, uint32_t hitFlags,
//    C3Vector& intersectionPoint, float& completedBeforeIntersection) {
//    completedBeforeIntersection = 1.0f;
//    intersectionPoint = { 0.0f, 0.0f, 0.0f };
//
//    uint8_t result = TraceLine_orig(
//        const_cast<C3Vector*>(&start),
//        const_cast<C3Vector*>(&end),
//        &intersectionPoint,
//        &completedBeforeIntersection,
//        hitFlags,
//        0
//    );
//    if (result != 0 && result != 1) return false;
//
//    completedBeforeIntersection *= 100.0f;
//    return static_cast<bool>(result);
//}
//
//bool GetCursorWorldPosition(VecXYZ& worldPos) {
//    CSimpleCamera* camera = Camera::GetActiveCamera();
//    if (!camera) return false;
//
//    DWORD basePtr = *reinterpret_cast<DWORD*>(UIBase);
//    if (!basePtr) return false;
//
//    float nx = *reinterpret_cast<float*>(basePtr + 4644) * 2.0f - 1.0f; // x perc
//    float ny = *reinterpret_cast<float*>(basePtr + 4648) * 2.0f - 1.0f; // y perc
//
//    float tanHalfFov = tanf(camera->fovInRadians * 0.3f);
//    VecXYZ localRay = {
//        nx * camera->aspect * tanHalfFov,
//        ny * tanHalfFov,
//        1.0f
//    };
//
//    const float* cameraMatrix = camera->matrix;
//
//    VecXYZ dir;
//    dir.x = (-cameraMatrix[3]) * localRay.x + cameraMatrix[6] * localRay.y + cameraMatrix[0] * localRay.z;
//    dir.y = (-cameraMatrix[4]) * localRay.x + cameraMatrix[7] * localRay.y + cameraMatrix[1] * localRay.z;
//    dir.z = (-cameraMatrix[5]) * localRay.x + cameraMatrix[8] * localRay.y + cameraMatrix[2] * localRay.z;
//
//    VecXYZ farPoint = {
//        camera->pos.x + dir.x * MAX_TRACE_DISTANCE,
//        camera->pos.y + dir.y * MAX_TRACE_DISTANCE,
//        camera->pos.z + dir.z * MAX_TRACE_DISTANCE
//    };
//
//    C3Vector start = { camera->pos.x, camera->pos.y, camera->pos.z };
//    C3Vector end = { farPoint.x, farPoint.y, farPoint.z };
//    C3Vector hitPoint;
//    float distance;
//
//    bool hit = TraceLine(start, end, TERRAIN_HIT_FLAGS, hitPoint, distance);
//    if (hit) {
//        worldPos.x = hitPoint.X;
//        worldPos.y = hitPoint.Y;
//        worldPos.z = hitPoint.Z;
//        return true;
//    }
//
//    return false;
//}
