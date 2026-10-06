#include "CommandLine.h"

#include <shellapi.h>

#include <algorithm>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "Extensions.h"
#include "Utils.h"

#include "include/CVar/CVar.h"
#include "include/Glue/LoginUI.h"
#include "include/Net/NetClient.h"

namespace {
inline constexpr utils::Accessor<std::vector<std::string>, struct ArgsTag> kArgs;

std::string_view flagName(std::string_view arg) {
    if (!arg.starts_with('-') && !arg.starts_with('/')) { return {}; }
    arg.remove_prefix(1);
    if (arg.starts_with('-')) { arg.remove_prefix(1); }
    return arg;
}

const std::string* getParam(std::string_view item) {
    if (item.empty() || kArgs->empty()) { return nullptr; }
    const std::span tail = std::span(*kArgs).subspan(1);
    const auto key = std::ranges::adjacent_find(
        tail, [item](const std::string& arg, const std::string&) { return flagName(arg) == item; });
    return key == tail.end() ? nullptr : &*std::next(key);
}

void setCVarFromParam(std::string_view param_name, const char* cvar_name) {
    const std::string* val = getParam(param_name);
    if (val == nullptr) { return; }
    if (CVar* cvar = CVar::find{}(cvar_name)) { cvar->setValue(val->c_str(), 1, 0, 0, 1); }
}

void onCharEnum() {
    static bool once = false;
    if (once) { return; }

    const std::string* character = getParam("character");
    if (character == nullptr) { return; }

    login::CharacterSelectionDisplay::TSGRA.enumerate([&](const login::CharacterSelectionDisplay& chr, uint32_t idx) {
        if (*character != std::string_view(std::data(chr.name))) { return true; }
        once = true;
        login::enterWorld(static_cast<int>(idx));
        return false;
    });
}

void onPostLoad() {
    static bool once = false;
    if (once) { return; }
    once = true;

    setCVarFromParam("realmlist", "realmList");
    setCVarFromParam("realmname", "realmName");

    const std::string* user = getParam("login");
    const std::string* pass = getParam("password");
    if (user != nullptr && pass != nullptr) { net_client::login{}(user->c_str(), pass->c_str()); }
}
}  // namespace

void command_line::initialize(hookkit::HookTransaction&) {
    int argc = 0;
    if (wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc)) {
        kArgs->reserve(static_cast<size_t>(argc));
        std::ranges::transform(
            std::span(argv, static_cast<size_t>(argc)), std::back_inserter(*kArgs), utils::wideToUtf8);
        LocalFree(argv);
    }

    extensions::glue::kCharEnum->add(onCharEnum);
    extensions::glue::kPostLoad->add(onPostLoad);
}
