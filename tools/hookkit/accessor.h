#pragma once

#include <atomic>
#include <string_view>
#include <type_traits>
#include <utility>

#include "abi_macros.h"

namespace utils {
template <typename T>
struct DefaultConstruct {
    T operator()() const { return T{}; }
};

namespace detail {
template <typename U>
consteval std::string_view typeSignature() {
#ifdef _MSC_VER
    return __FUNCSIG__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

template <auto V>
consteval std::string_view valueSignature() {
#ifdef _MSC_VER
    return __FUNCSIG__;
#else
    return __PRETTY_FUNCTION__;
#endif
}

consteval bool namesClosure(std::string_view signature) {
    return signature.find("<lambda_") != std::string_view::npos ||
           signature.find("(lambda") != std::string_view::npos;
}

template <typename T, auto Factory>
inline constexpr bool kIsNamedFactory =
    std::is_same_v<std::remove_cv_t<decltype(Factory)>, DefaultConstruct<T>> ||
    (std::is_pointer_v<decltype(Factory)> && std::is_function_v<std::remove_pointer_t<decltype(Factory)>>);
}  // namespace detail

// tag keys the instance: every variable needs its own named tag type, or all Accessors of one T share an instance
template <typename T, typename Tag, auto Factory = DefaultConstruct<T>{}>
class Accessor {
public:
    // a closure type gets a per-TU name, which would split one variable into a separate instance per TU
    static_assert(!detail::namesClosure(detail::typeSignature<Tag>()),
        "utils::Accessor: Tag must be a named type (struct FooTag), not a lambda");
    static_assert(detail::kIsNamedFactory<T, Factory>,
        "utils::Accessor: Factory must be a named function pointer (or omitted), not a lambda");
    static_assert(!detail::namesClosure(detail::valueSignature<Factory>()),
        "utils::Accessor: Factory must be a named function, not a converted lambda");

    static HOOKKIT_FORCEINLINE T& get() {
        if (T* ptr = instance_.load(std::memory_order_acquire)) [[likely]] { return *ptr; }
        return getSlow();
    }

    HOOKKIT_FORCEINLINE T* operator->() const { return &get(); }

    T& operator*() const { return get(); }

    explicit operator T&() const { return get(); }

    template <typename Key>
    decltype(auto) operator[](Key&& idx) const {
        return get()[std::forward<Key>(idx)];
    }

private:
    __declspec(noinline) static T& getSlow() {
        static T& instance = *new T(Factory());
        instance_.store(&instance, std::memory_order_release);
        return instance;
    }

    inline static constinit std::atomic<T*> instance_{nullptr};
};
}  // namespace utils
