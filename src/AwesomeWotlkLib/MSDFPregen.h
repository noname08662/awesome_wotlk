#pragma once

#include <ft2build.h>
#include <share.h>
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include FT_FREETYPE_H

#include "MSDFUtils.h"

class Throttle {
public:
    explicit Throttle(double target_percent)
        : target_usage_(std::clamp(target_percent, 1.0, 100.0)), last_sleep_(std::chrono::steady_clock::now()) {}

    void startWork() { last_sleep_ = std::chrono::steady_clock::now(); }

    void endWork(const std::atomic<bool>* cancel = nullptr) {
        const auto now = std::chrono::steady_clock::now();
        accumulated_work_ += std::chrono::duration_cast<std::chrono::milliseconds>(now - last_sleep_);

        if (accumulated_work_.count() >= kSleepThresholdMs) {
            const auto work_ms = static_cast<double>(accumulated_work_.count());
            int sleep_ms = static_cast<int>(work_ms / (target_usage_ / 100.0) - work_ms);
            while (sleep_ms > 0) {
                if (cancel != nullptr && cancel->load(std::memory_order_acquire)) { break; }
                const int chunk = std::min(sleep_ms, kSleepChunkMs);
                std::this_thread::sleep_for(std::chrono::milliseconds(chunk));
                sleep_ms -= chunk;
            }
            accumulated_work_ = std::chrono::milliseconds{0};
        }
        last_sleep_ = std::chrono::steady_clock::now();
    }

private:
    static constexpr int64_t kSleepThresholdMs = 100;
    static constexpr int kSleepChunkMs = 50;

    double target_usage_;
    std::chrono::steady_clock::time_point last_sleep_;
    std::chrono::milliseconds accumulated_work_{0};
};

struct ConsoleGuard {
    FILE* fp_out = nullptr;
    FILE* fp_in = nullptr;
    HWND wnd = nullptr;
    bool own_console = false;
    bool allocated = false;

    ConsoleGuard()
        : wnd(GetActiveWindow()),
          own_console(AllocConsole() != 0),
          allocated(own_console || GetLastError() == ERROR_ACCESS_DENIED) {
        if (allocated) {
            fp_out = _wfsopen(L"CONOUT$", L"w", _SH_DENYNO);
            fp_in = _wfsopen(L"CONIN$", L"r", _SH_DENYNO);
            if (fp_out != nullptr) { static_cast<void>(std::setvbuf(fp_out, nullptr, _IONBF, 0)); }
            if (fp_in != nullptr) { static_cast<void>(std::setvbuf(fp_in, nullptr, _IONBF, 0)); }
            if (own_console) {
                if (HMENU menu = GetSystemMenu(GetConsoleWindow(), FALSE); menu != nullptr) {
                    DeleteMenu(menu, SC_CLOSE, MF_BYCOMMAND);
                }
            }
            if (wnd != nullptr) { ShowWindow(wnd, SW_MINIMIZE); }
            SetForegroundWindow(GetConsoleWindow());
        }
    }

    ~ConsoleGuard() {
        if (fp_out != nullptr) { static_cast<void>(fclose(fp_out)); }
        if (fp_in != nullptr) { static_cast<void>(fclose(fp_in)); }
        if (own_console) { FreeConsole(); }
        if (wnd != nullptr) {
            ShowWindow(wnd, SW_RESTORE);
            SetForegroundWindow(wnd);
        }
    }

    ConsoleGuard(const ConsoleGuard&) = delete;
    ConsoleGuard& operator=(const ConsoleGuard&) = delete;
    ConsoleGuard(ConsoleGuard&&) = delete;
    ConsoleGuard& operator=(ConsoleGuard&&) = delete;
};

class MSDFPregen {
public:
    static bool tryStartPreGen() noexcept;
    static void shutdown() noexcept;

    static bool isPreGenRunning() { return pregen_lock_file_ != INVALID_HANDLE_VALUE; }

private:
    struct PreGenRequest {
        FT_Face face = nullptr;
        const FT_Byte* data = nullptr;
        FT_Long size = 0;
        FT_Long face_index = 0;
        std::string family_name;
        std::string style_name;
        FontHash hash = 0;
        std::filesystem::path path;
        std::string addon_name;
        std::vector<std::string> names;
        std::string variant;
    };

    struct GenerateSettings {
        uint32_t start = 0;
        uint32_t end = 0;
        double cpu_limit = 100.0;
    };

    enum class GenerateOutcome { eComplete, eSkipped, eFailed, eCancelled };

    static std::vector<PreGenRequest> collectRequests();
    static void executePreGeneration();
    static bool acquirePreGenLock();
    static void releasePreGenLock();
    static std::optional<GenerateSettings> promptSettings(const std::string& title);
    static GenerateOutcome generateFont(const PreGenRequest& req, const GenerateSettings& settings);
    static void refreshLoadedFonts(const PreGenRequest& req);
    static int WINAPI consoleCtrlHandler(DWORD ctrl_type);
    static void flushStdin();

    inline static auto pregen_lock_file_ = INVALID_HANDLE_VALUE;
    inline static std::atomic<bool> cancel_pregen_{false};
    inline static HANDLE pregen_finished_event_ = nullptr;
};
