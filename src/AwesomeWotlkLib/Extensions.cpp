#include "Extensions.h"

#include <Hookkit.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "include/BaseTypes.h"
#include "include/CVar/CVar.h"
#include "include/Game/CGGameUI.h"
#include "include/Glue/CGlueMgr.h"
#include "include/Lib/Lua.h"

namespace extensions {
namespace console {
void CVarRegistry::refresh(std::string_view name) const {
    const auto it = index_.find(name);
    if (it == index_.end()) { return; }
    Entry& entry = *it->second;
    if (entry.cvar == nullptr || entry.cvar->str_ == nullptr) { return; }
    thunk(entry.cvar, nullptr, entry.cvar->str_, &entry);
}

void CVarRegistry::set(std::string_view name, const char* raw_value) const {
    const auto it = index_.find(name);
    if (it == index_.end()) { return; }
    Entry& entry = *it->second;
    if (entry.cvar == nullptr) { return; }
    entry.cvar->set(raw_value, true, false, false, true);
    thunk(entry.cvar, nullptr, entry.cvar->str_, &entry);
}

void CVarRegistry::registerAll() {
    for (Entry& entry : entries_) {
        entry.cvar =
            CVar::reg{}(entry.name, entry.desc, entry.flags, entry.init_value.c_str(), &thunk, 0, false, &entry, false);
    }
}

int CVarRegistry::thunk(CVar* cvar, const char*, const char* new_value, void* user_data) {
    auto& entry = *static_cast<Entry*>(user_data);
    return entry.apply(entry, cvar, new_value);
}
}  // namespace console

namespace framescript {
bool TokenRegistry::resolveGuid(const char** stack_ptr, guid_t* out_guid) const {
    return std::ranges::any_of(tokens_, [&](const auto& entry) {
        const auto& [token, conv] = entry;
        if (std::strncmp(*stack_ptr, token.data(), token.size()) != 0) { return false; }
        *stack_ptr = std::next(*stack_ptr, static_cast<ptrdiff_t>(token.size()));
        if (std::holds_alternative<IndexedToken>(conv)) {
            const auto& [guid_getter, id_getter] = std::get<IndexedToken>(conv);
            uint32_t n = storm::str::toUnsignedPtr{}(stack_ptr);
            *out_guid = n > 0 ? guid_getter(static_cast<int>(n - 1u)) : 0;
        } else {
            const auto& [guid_getter, id_getter] = std::get<SingleToken>(conv);
            *out_guid = guid_getter();
        }
        return true;
    });
}

void TokenRegistry::fillTokens(const guid_t* guid, char** buf, size_t* size) const {
    for (const auto& [token, conv] : tokens_) {
        if (*size >= 8) { break; }
        char* dst = *std::next(buf, static_cast<ptrdiff_t>(*size));
        std::optional<std::string> text;
        if (std::holds_alternative<IndexedToken>(conv)) {
            const auto& [guid_getter, id_getter] = std::get<IndexedToken>(conv);
            int id = id_getter(*guid);
            if (id >= 0) { text = std::format("{}{}", token, id + 1); }
        } else {
            const auto& [guid_getter, id_getter] = std::get<SingleToken>(conv);
            if (id_getter(*guid)) { text = token; }
        }
        if (!text) { continue; }
        const size_t len = std::min(text->size(), size_t{31});
        std::memcpy(dst, text->data(), len);
        dst[len] = '\0';
        ++(*size);
    }
}
}  // namespace framescript

namespace {

HOOKKIT_BIND(framescript::fillEvents_hook, [](const char** list, size_t count) {
    std::vector events(list, std::next(list, static_cast<ptrdiff_t>(count)));
    console::kEventRegistry->fillInto(events);
    framescript::fillEvents{}(events.data(), events.size());
});

HOOKKIT_BIND(CVar::init_hook, []() {
    CVar::init{}();
    console::kCvarRegistry->registerAll();
});

HOOKKIT_BIND(framescript::paintCallback_hook, [](int unused1, int unused2, int unused3, float elapsed) {
    framescript::kOnUpdate->fire();
    return framescript::paintCallback{}(unused1, unused2, unused3, elapsed);
});

HOOKKIT_BIND(game_ui::enterWorld_hook, []() {
    framescript::kOnEnter->fire();
    return game_ui::enterWorld{}();
});

HOOKKIT_BIND(game_ui::leaveWorld_hook, []() {
    framescript::kOnLeave->fire();
    return game_ui::leaveWorld{}();
});

HOOKKIT_HOOK_BIND(openFrameXML_site, 0x0051226D, hookkit::Conv::eCdecl, void)[]() {
    framescript::registerBNScriptFunctions{}();

    if (LuaState* l = lua::getLuaState()) { console::kLuaLibRegistry->loadInto(l); }
};

HOOKKIT_BIND(framescript::getTokensFromGuid_hook, [](guid_t* guid, size_t* size) {
    char** buf = framescript::getTokensFromGuid{}(guid, size);
    if (!buf) { return buf; }
    framescript::kTokenRegistry->fillTokens(guid, buf, size);
    return buf;
});

HOOKKIT_NAMED_BIND_RAW(getGuidFromToken_site, 0x0060AFAA, {"resolved", 0x0060AD57}, {"orig", 0x0060AD44}) {
    constexpr uintptr_t kOrig = getGuidFromToken_site::target("orig");
    constexpr uintptr_t kResolved = getGuidFromToken_site::target("resolved");
    const uintptr_t kBulk = HOOKKIT_LAMBDA_ADDR([](const char** stack_ptr, guid_t* out_guid) {
        return framescript::kTokenRegistry->resolveGuid(stack_ptr, out_guid);
    });
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .step(pushAllStep())
        .step(pushMem(Reg::eBp, 0xC))
        .step(pushLea(Reg::eBp, 0x8))
        .build(kBulk, branchOnResult(ResultWidth::eByte, jmpTo(kResolved), jmpTo(kOrig), popAllStep()));
};

HOOKKIT_BIND_RAW(glue::loadGlueXML_site, ([] {
    constexpr uintptr_t kResumeStatusDtor = glue::loadGlueXML_site::target("resumeStatusDtor");
    constexpr uintptr_t kJmpback = glue::loadGlueXML_site::target("jmpback");
    const uintptr_t kBulk = HOOKKIT_LAMBDA_ADDR([]() { glue::kPostLoad->fire(); });
    return CallsiteTrampolineBuilder{}.assertOnBuildFailure().build(
        kResumeStatusDtor, jmpTo(kJmpback, pushAllStep(), callTo(kBulk), popAllStep()), 0);
}()));

HOOKKIT_BIND_RAW(glue::loadCharacters_site, ([] {
    const uintptr_t kBulk = HOOKKIT_LAMBDA_ADDR([]() { glue::kCharEnum->fire(); });
    return CallsiteTrampolineBuilder{}
        .assertOnBuildFailure()
        .step(addReg(Reg::eSp, 8))
        .step(popReg(Reg::eSi))
        .step(pushAllStep())
        .build(kBulk, ret(popAllStep()), 0);
}()));
}  // namespace
}  // namespace extensions

void extensions::initialize(hookkit::HookTransaction& tx) {
    tx.attach(CVar::init_hook{}, framescript::paintCallback_hook{}, framescript::fillEvents_hook{},
        framescript::getTokensFromGuid_hook{}, getGuidFromToken_site{}, openFrameXML_site{}, game_ui::enterWorld_hook{},
        game_ui::leaveWorld_hook{}, glue::loadGlueXML_site{}, glue::loadCharacters_site{});
}
