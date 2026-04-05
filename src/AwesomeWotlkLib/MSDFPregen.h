#pragma once
#include "MSDF.h"
#include "MSDFCache.h"

class Throttle {
private:
    double targetUsage;
    std::chrono::steady_clock::time_point lastSleep;
    std::chrono::milliseconds accumulatedWork{ 0 };

public:
    Throttle(double targetPercent)
        : targetUsage(std::clamp(targetPercent, 1.0, 100.0)),
        lastSleep(std::chrono::steady_clock::now()) {
    }
    void StartWork() {
        lastSleep = std::chrono::steady_clock::now();
    }
    void EndWork() {
        auto now = std::chrono::steady_clock::now();
        auto workDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSleep);
        accumulatedWork += workDuration;

        if (accumulatedWork.count() >= 100) {
            int sleepMs = static_cast<int>(accumulatedWork.count() / (targetUsage / 100.0) - accumulatedWork.count());
            if (sleepMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
            }
            accumulatedWork = std::chrono::milliseconds{ 0 };
        }
        lastSleep = std::chrono::steady_clock::now();
    }
};

struct ConsoleGuard {
    FILE* fpOut = nullptr;
    FILE* fpIn = nullptr;
    bool allocated = false;
    HWND wnd = nullptr;

    ConsoleGuard() {
        wnd = GetActiveWindow();
        allocated = AllocConsole() || GetLastError() == ERROR_ACCESS_DENIED;
        if (allocated) {
            freopen_s(&fpOut, "CONOUT$", "w", stdout);
            freopen_s(&fpIn, "CONIN$", "r", stdin);
            if (wnd) {
                ShowWindow(wnd, SW_MINIMIZE);
            }
            SetForegroundWindow(GetConsoleWindow());
        }
    }
    ~ConsoleGuard() {
        if (fpOut) fclose(fpOut);
        if (fpIn) fclose(fpIn);
        if (allocated) FreeConsole();
        if (wnd) {
            ShowWindow(wnd, SW_RESTORE);
            SetForegroundWindow(wnd);
        }
    }
};

class MSDFPregen {
public:
    static void RegisterForPreGen(FT_Face aface, const FT_Byte* data, FT_Long size, FT_Long faceIndex);
    static bool TryStartPreGen();
    static void Shutdown() noexcept;

private:
    struct ThreadLocalBatch {
        std::vector<std::pair<uint32_t, GlyphMetrics>> glyphs;
        std::vector<std::vector<uint8_t>> ownedBuffers;
        std::mutex mutex;
        size_t memoryUsed = 0;
    };

    struct PreGenRequest {
        FT_Face face = nullptr;
        const FT_Byte* data = nullptr;
        FT_Long size = 0;
        FT_Long faceIndex = 0;
        std::string familyName;
        std::string styleName;
    };

    static void ExecutePreGeneration();
    static bool AcquirePreGenLock();
    static void ReleasePreGenLock();
    static bool GenerateFont(const PreGenRequest& req);
    static void FlushStdin() {
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }

    inline static std::vector<PreGenRequest> s_pendingRequests;
    inline static HANDLE s_pregenLockFile = INVALID_HANDLE_VALUE;
};