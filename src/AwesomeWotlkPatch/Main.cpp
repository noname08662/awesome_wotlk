#include "Utils.h"
#include "Patch.h"
#include <Windows.h>
#include <filesystem>
#include <format>
#include <string>
#define AWESOMEWOTLKLIB_DLL "AwesomeWotlkLib.dll"

namespace {
    std::string s_appName;
    bool s_quietMode = false;

    const char* findGameClientExecutable() {
        static const char* possibleNames[] = { "Project-Epoch.exe", "Wow.exe" };
        for (const char* name : possibleNames) if (std::filesystem::is_regular_file(name)) return name;
        return nullptr;
    }

    bool applyPatches(const char* path) {
        std::vector<char> image;
        if (!readFile(path, image)) return false;
    	if (!applyLAA(image)) {
    		SetLastError(ERROR_INVALID_DATA);
    		return false;
    	}
        for (auto& [virtualAddress, hexBytes] : s_patches) {
            unsigned offset = virtualAddress2RawOffset(image.data(), virtualAddress);
            if (!offset) {
                SetLastError(ERROR_INVALID_ADDRESS);
                return false;
            }
            std::vector<char> data;
            if (!convHexString2ByteArray(hexBytes, data) || data.empty()) {
                SetLastError(ERROR_BAD_ARGUMENTS);
                return false;
            }
            memcpy(&image[offset], data.data(), data.size());
        }
        return writeFile(path, image);
    }

    template<typename... Args>
    void message(DWORD icon, const char* fmt, Args&&... args) {
        if (!s_quietMode) {
            MessageBoxA(nullptr, std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...)).c_str(), s_appName.c_str(), icon | MB_OK);
        }
    }
}

int main(int argc, char** argv) {
    s_appName = std::filesystem::path(argv[0]).filename().string();

    const char* exePath = nullptr;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0) {
            s_quietMode = true;
        }
        else if (!exePath) {
            exePath = argv[i];
        }
    }
    if (!exePath) exePath = findGameClientExecutable();
    if (!exePath) {
        message(MB_ICONERROR,
            "World of Warcraft executable not found!\n"
            "You must do one of this action:\n"
            "- Move patcher to folder with Wow.exe\n"
            "- Drag and drop Wow.exe into {}\n"
            "- Use command line interface `{} <path>`",
            s_appName, s_appName);
        return 1;
    }

    if (!applyPatches(exePath)) {
        DWORD lastError = GetLastError();
        const char* msg = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&msg), 0, nullptr);
        message(MB_ICONERROR,
            "Can't apply patch to {} - {} - {}",
            exePath, msg, lastError);
        return 1;
    }

    std::filesystem::path libInGamePath = std::filesystem::path(exePath).parent_path() / AWESOMEWOTLKLIB_DLL;
    std::filesystem::path libInAppPath = std::filesystem::absolute(argv[0]).parent_path() / AWESOMEWOTLKLIB_DLL;
    if (!std::filesystem::is_regular_file(libInGamePath) && std::filesystem::is_regular_file(libInAppPath)) {
        std::error_code ec;
        std::filesystem::copy_file(libInAppPath, libInGamePath, ec);
    }

    if (std::filesystem::is_regular_file(libInGamePath)) {
        message(MB_ICONINFORMATION,
            "Patch succesfully applied on {} \n"
            "Now you can enter the game",
            exePath
        );
    }
    else {
        message(MB_ICONWARNING,
            "Patch succesfully applied on {}\n"
            "But look like `" AWESOMEWOTLKLIB_DLL "` is missed\n"
            "Before entering game you must place it to game folder",
            exePath
        );
    }
    return 0;
}