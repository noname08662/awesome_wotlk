#pragma once
#include "GameClient.h"
#include <Windows.h>
#include <string>
#include <string_view>

inline std::string u16tou8(std::wstring_view u16) {
    int u8len = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, u16.data(), u16.size() + 1, nullptr, 0, nullptr, nullptr);
    if (!u8len) return {};
    std::string u8;
    u8.resize(u8len);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, u16.data(), u16.size() + 1, u8.data(), u8len, nullptr, nullptr);
    return u8;
}

inline std::string GetFromClipboardU8(HWND hwnd) {
    if (!OpenClipboard(hwnd)) return {};
    HANDLE hMem = GetClipboardData(CF_UNICODETEXT);
    if (!hMem) {
    on_fail:
        CloseClipboard();
        return {};
    }
    const wchar_t* utf16 = (const wchar_t*)GlobalLock(hMem);
    if (!utf16) goto on_fail;
    int utf16Length = wcslen(utf16) + 1;
    int utf8Length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, utf16Length, nullptr, 0, nullptr, nullptr);
    if (utf8Length == 0) {
        GlobalUnlock(hMem);
        goto on_fail;
    }

    std::string utf8;
    utf8.resize(utf8Length);
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, utf16, utf16Length, &utf8[0], utf8Length, nullptr, nullptr);

    GlobalUnlock(hMem);
    CloseClipboard();
    return utf8;
}

inline bool CopyToClipboardU8(const char* u8Str, HWND hwnd) {
    if (!u8Str || !u8Str[0]) { // just empty
        if (!OpenClipboard(hwnd)) return false;
        bool result = EmptyClipboard();
        CloseClipboard();
        return result;
    }
    int u8CharsLen = strlen(u8Str) + 1;
    int wCharsLen = MultiByteToWideChar(CP_UTF8, 0, u8Str, u8CharsLen, 0, 0);

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, sizeof(wchar_t) * wCharsLen);
    if (!hMem) return false;

    wchar_t* cbBuf = (wchar_t*)GlobalLock(hMem);
    if (!cbBuf) {
    on_fail:
        GlobalFree(hMem);
        return false;
    }

    MultiByteToWideChar(CP_UTF8, 0, u8Str, u8CharsLen, cbBuf, wCharsLen);
    cbBuf[wCharsLen] = L'\0';
    GlobalUnlock(hMem);

    if (!OpenClipboard(hwnd)) goto on_fail;
    if (!EmptyClipboard()) {
        CloseClipboard();
        goto on_fail;
    }

    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    return true;
}

using gc_atoi_t = int(__stdcall*)(const char**);
inline int __stdcall gc_atoi(const char** str) { return reinterpret_cast<gc_atoi_t>(0x76F190)(str); }

inline HWND GetGameWindow() { return *reinterpret_cast<HWND*>(0x00D41620); }
inline bool IsInWorld() { return *reinterpret_cast<char*>(0x00BD0792); }

inline void dbg_printf(const char* fmt, ...) {
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, ap);
    va_end(ap);
    OutputDebugStringA(buf);
}


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