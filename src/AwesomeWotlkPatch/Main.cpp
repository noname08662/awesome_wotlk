#include <hookkit/accessor.h>
#include <windows.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <span>
#include <string>
#include <string_view>

#include "Patch.h"
#include "Utils.h"

namespace {
inline constexpr utils::Accessor<std::string, struct AppNameTag> kAppName;
inline constexpr utils::Accessor<bool, struct QuietModeTag> kQuietMode;

const char* findGameClientExecutable() {
    static constexpr std::array<const char*, 2> kPossibleNames = {
        "Project-Epoch.exe",
        "Wow.exe",
    };
    for (const char* name : kPossibleNames) {
        if (std::filesystem::is_regular_file(name)) { return name; }
    }
    return nullptr;
}

bool applyPatches(const char* path) {
    std::vector<char> image;
    if (!readFile(path, image)) { return false; }
    if (!applyLAA(image)) {
        SetLastError(ERROR_INVALID_DATA);
        return false;
    }
    for (const auto& [virtual_address, hex_bytes] : kPatches) {
        unsigned offset = virtualAddress2RawOffset(image, virtual_address);
        if (offset == 0u) {
            SetLastError(ERROR_INVALID_ADDRESS);
            return false;
        }
        std::vector<char> data;
        if (!convHexString2ByteArray(hex_bytes, data) || data.empty()) {
            SetLastError(ERROR_BAD_ARGUMENTS);
            return false;
        }
        if (offset + data.size() > image.size()) {
            SetLastError(ERROR_INVALID_ADDRESS);
            return false;
        }
        std::ranges::copy(data, image.begin() + static_cast<int>(offset));
    }
    return writeFile(path, image);
}

template <typename... Args>
void message(DWORD icon, const char* fmt, const Args&... args) {
    if (!*kQuietMode) {
        MessageBoxA(
            nullptr, std::vformat(fmt, std::make_format_args(args...)).c_str(), kAppName->c_str(), icon | MB_OK);
    }
}

int run(std::span<char*> args) {
    *kAppName = std::filesystem::path(args.front()).filename().string();
    const std::string& app_name = *kAppName;

    const char* exe_path = nullptr;
    for (const char* arg : args.subspan(1)) {
        if (std::string_view(arg) == "--quiet" || std::string_view(arg) == "-q") {
            *kQuietMode = true;
        } else if (exe_path == nullptr) {
            exe_path = arg;
        }
    }
    if (exe_path == nullptr) { exe_path = findGameClientExecutable(); }
    if (exe_path == nullptr) {
        message(MB_ICONERROR,
            "World of Warcraft executable (Wow.exe) not found.\n"
            "Do one of the following:\n"
            "- Move the patcher into the folder that contains Wow.exe\n"
            "- Drag and drop Wow.exe onto {}\n"
            "- Run `{} <path>` from the command line",
            app_name, app_name);
        return 1;
    }

    if (!applyPatches(exe_path)) {
        DWORD last_error = GetLastError();
        char* system_message = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, last_error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&system_message), 0,
            nullptr);
        message(MB_ICONERROR, "Failed to patch {}.\n{}\n(error code {})", exe_path,
            system_message != nullptr ? system_message : "Unknown error.", last_error);
        LocalFree(system_message);
        return 1;
    }

    std::filesystem::path lib_in_game_path = std::filesystem::path(exe_path).parent_path() / "AwesomeWotlkLib.dll";
    std::filesystem::path lib_in_app_path =
        std::filesystem::absolute(args.front()).parent_path() / "AwesomeWotlkLib.dll";
    if (!std::filesystem::is_regular_file(lib_in_game_path) && std::filesystem::is_regular_file(lib_in_app_path)) {
        std::error_code ec;
        std::filesystem::copy_file(lib_in_app_path, lib_in_game_path, ec);
    }

    if (std::filesystem::is_regular_file(lib_in_game_path)) {
        message(MB_ICONINFORMATION,
            "Successfully patched {}.\n"
            "You can now start the game.",
            exe_path);
    } else {
        message(MB_ICONWARNING,
            "Successfully patched {}, but `{}` was not found.\n"
            "Place it in the game folder before starting the game.",
            exe_path, "AwesomeWotlkLib.dll");
    }
    return 0;
}
}  // namespace

int main(int argc, char** argv) {
    try {
        return run(std::span(argv, static_cast<size_t>(argc)));
    } catch (...) { return 1; }
}
