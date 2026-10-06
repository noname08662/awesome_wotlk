#pragma once

#include <ankerl/unordered_dense.h>
#include <hookkit/accessor.h>
#include <hookkit/transaction.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <format>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "include/BaseTypes.h"
#include "include/CVar/CVar.h"
#include "include/FrameScript/FrameScript.h"
#include "include/Glue/CGlueMgr.h"
#include "include/Lib/Lua.h"

namespace extensions {
void initialize(hookkit::HookTransaction& tx);

class CallbackList {
public:
    void add(FunctionCallback func) { callbacks_.push_back(std::move(func)); }

    void fire() const {
        for (const auto& f : callbacks_) {
            f();
        }
    }

private:
    std::vector<FunctionCallback> callbacks_;
};

namespace console {
template <typename T>
class Bound {
public:
    explicit constexpr Bound(T value) : value_(value) {}

    template <typename F>
        requires(!std::is_arithmetic_v<F> && std::is_convertible_v<F, T (*)()>)
    explicit constexpr Bound(F fn) : fn_(fn) {}

    [[nodiscard]]
    T operator()() const {
        return fn_ != nullptr ? fn_() : value_;
    }

private:
    T value_{};
    T (*fn_)() = nullptr;
};

// fires on every engine set, changed is false when the value equals the last applied one (true on the first dispatch)
// accepts (value, changed), value-only and nullary callbacks
template <typename T>
class OnChange : public std::function<void(T, bool)> {
public:
    OnChange() = default;

    template <typename F>
        requires std::is_invocable_v<F&, T, bool>
    OnChange(F fn) : std::function<void(T, bool)>(std::move(fn)) {}

    template <typename F>
        requires(!std::is_invocable_v<F&, T, bool> && std::is_invocable_v<F&, T>)
    OnChange(F fn) : std::function<void(T, bool)>([fn = std::move(fn)](T value, bool) mutable { fn(value); }) {}

    template <typename F>
        requires(!std::is_invocable_v<F&, T, bool> && !std::is_invocable_v<F&, T> && std::is_invocable_v<F&>)
    OnChange(F fn) : std::function<void(T, bool)>([fn = std::move(fn)](T, bool) mutable { fn(); }) {}
};

template <typename T>
struct CVarDesc {
    const char* name = nullptr;
    const char* desc = nullptr;
    T init;
    Bound<T> min{std::numeric_limits<T>::lowest()};
    Bound<T> max{std::numeric_limits<T>::max()};
    const char* fmt = std::is_floating_point_v<T> ? "%.2f" : "%d";
    CVar::CVarFlags flags{};
    OnChange<T> on_change;
};

// compile-time string usable as a non-type template parameter
template <size_t N>
struct FixedString {
    std::array<char, N> buf{};

    constexpr FixedString(const char (&s)[N]) { std::ranges::copy(s, buf.begin()); }

    [[nodiscard]]
    constexpr std::string_view view() const {
        return {buf.data(), N - 1};
    }
};

class CVarRegistry {
public:
    template <typename T>
    void add(CVarDesc<T> desc) {
        static_assert(std::is_same_v<T, int> || std::is_same_v<T, float>, "cvar values are int or float");
        assert(!index_.contains(std::string_view{desc.name}) && "duplicate cvar name");
        Entry& entry = entries_.emplace_back();
        entry.name = desc.name;
        entry.desc = desc.desc;
        entry.flags = desc.flags;
        entry.init_value = std::format("{}", desc.init);
        entry.value = desc.init;
        entry.apply = [desc = std::move(desc)](Entry& e, CVar* cvar, const char* raw_value) {
            T value = std::get<T>(e.value);
            const int result = cvar->sync(raw_value, &value, desc.min(), desc.max(), desc.fmt);
            // compared after sync, a clamp re-enters through the engine's set and stores the clamped value first
            const bool changed = !e.applied || value != std::get<T>(e.value);
            e.value = value;
            if (raw_value != nullptr) {
                e.applied = true;
                if (desc.on_change) { desc.on_change(value, changed); }
            }
            return result;
        };
        index_[entry.name] = &entry;
    }

    template <typename T>
    [[nodiscard]]
    const T& ref(std::string_view name) const {
        static const T fallback{};
        const auto it = index_.find(name);
        assert(it != index_.end() && "unknown cvar");
        if (it == index_.end()) { return fallback; }
        const T* value = std::get_if<T>(&it->second->value);
        assert(value != nullptr && "cvar type mismatch");
        return value != nullptr ? *value : fallback;
    }

    template <FixedString Name, typename T>
    [[nodiscard]]
    const T& ref() const {
        static const T& cached = ref<T>(Name.view());
        return cached;
    }

    template <typename T>
    [[nodiscard]]
    T get(std::string_view name) const {
        return ref<T>(name);
    }

    template <FixedString Name, typename T>
    [[nodiscard]]
    T get() const {
        return ref<Name, T>();
    }

    void refresh(std::string_view name) const;

    void set(std::string_view name, const char* raw_value) const;

    void registerAll();

private:
    struct Entry {
        const char* name = nullptr;
        const char* desc = nullptr;
        CVar::CVarFlags flags{};
        std::string init_value;
        std::variant<int, float> value;
        bool applied = false;  // a value was dispatched at least once
        std::function<int(Entry&, CVar*, const char*)> apply;
        CVar* cvar = nullptr;
    };

    static int thunk(CVar* cvar, const char* prev_value, const char* new_value, void* user_data);

    struct NameHash {
        using is_transparent = void;
        using is_avalanching = void;

        uint64_t operator()(std::string_view name) const noexcept {
            return ankerl::unordered_dense::hash<std::string_view>{}(name);
        }
    };

    std::deque<Entry> entries_;
    ankerl::unordered_dense::map<std::string, Entry*, NameHash, std::equal_to<>> index_;
};

class EventRegistry {
public:
    static constexpr int kUnassigned = -1;

    const int& add(const char* name) {
        index_[name] = names_.size();
        names_.push_back(name);
        return ids_.emplace_back(kUnassigned);
    }

    // no-op until the engine assigned the ids
    template <typename... Args>
    void fire(std::string_view name, const char* fmt, Args... args) const {
        if (const int id = idOf(name); id != kUnassigned) { ::framescript::event(id, fmt, args...); }
    }

    // args already on the lua stack
    void signal(std::string_view name, LuaState* l, int nargs) const {
        if (const int id = idOf(name); id != kUnassigned) { ::framescript::signalEvent{}(id, l, nargs); }
    }

    void fillInto(std::vector<const char*>& out) {
        out.reserve(out.size() + names_.size());
        const auto base = static_cast<int>(out.size());
        out.insert(out.end(), names_.begin(), names_.end());
        for (size_t i = 0; i < ids_.size(); ++i) {
            ids_[i] = base + static_cast<int>(i);
        }
    }

private:
    [[nodiscard]]
    int idOf(std::string_view name) const {
        const auto it = index_.find(name);
        assert(it != index_.end() && "unknown event");
        return it != index_.end() ? ids_[it->second] : kUnassigned;
    }

    std::vector<const char*> names_;
    std::deque<int> ids_;
    ankerl::unordered_dense::map<std::string_view, size_t> index_;
};

class LuaLibRegistry {
public:
    void add(const lua::LuaCFunction& func) { libs_.push_back(func); }

    void loadInto(LuaState* l) const {
        for (const auto& func : libs_) {
            func(l);
        }
    }

private:
    std::vector<lua::LuaCFunction> libs_;
};

inline constexpr utils::Accessor<CVarRegistry, struct CvarRegistryTag> kCvarRegistry;
inline constexpr utils::Accessor<EventRegistry, struct EventRegistryTag> kEventRegistry;
inline constexpr utils::Accessor<LuaLibRegistry, struct LuaLibRegistryTag> kLuaLibRegistry;

}  // namespace console

namespace framescript {
using namespace ::framescript;

class TokenRegistry {
public:
    using TokenGuidGetter = guid_t();
    using TokenNGuidGetter = guid_t(int);
    using TokenIdGetter = bool(guid_t);
    using TokenIdNGetter = int(guid_t);

    void add(const char* token, TokenGuidGetter* guid, TokenIdGetter* id) { tokens_[token] = SingleToken{guid, id}; }

    void add(const char* token, TokenNGuidGetter* guid, TokenIdNGetter* id) { tokens_[token] = IndexedToken{guid, id}; }

    bool resolveGuid(const char** stack_ptr, guid_t* out_guid) const;
    void fillTokens(const guid_t* guid, char** buf, size_t* size) const;

private:
    using SingleToken = std::pair<TokenGuidGetter*, TokenIdGetter*>;
    using IndexedToken = std::pair<TokenNGuidGetter*, TokenIdNGetter*>;
    using TokenDetails = std::variant<SingleToken, IndexedToken>;
    ankerl::unordered_dense::map<std::string, TokenDetails> tokens_;
};

inline constexpr utils::Accessor<TokenRegistry, struct TokenRegistryTag> kTokenRegistry;

inline constexpr utils::Accessor<CallbackList, struct OnUpdateTag> kOnUpdate;
inline constexpr utils::Accessor<CallbackList, struct OnEnterTag> kOnEnter;
inline constexpr utils::Accessor<CallbackList, struct OnLeaveTag> kOnLeave;

}  // namespace framescript

namespace glue {
using namespace ::glue;

inline constexpr utils::Accessor<CallbackList, struct PostLoadTag> kPostLoad;
inline constexpr utils::Accessor<CallbackList, struct CharEnumTag> kCharEnum;

}  // namespace glue
}  // namespace extensions
