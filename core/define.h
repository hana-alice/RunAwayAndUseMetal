#pragma once
#include <boost/container/container_fwd.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#ifndef RAUM_RELEASE
    #define RAUM_DEBUG 1
static constexpr bool raum_debug{true};
    #include <source_location>
    #include "utils/log.h"
#else

static constexpr bool raum_debug{false};
#endif

#if defined(_WIN32)
    #define RAUM_WINDOWS 1
#endif

#ifdef RAUM_DEBUG
    #define DEBUG(exp) exp
#else
    #define DEBUG(exp)
#endif

#ifdef RAUM_WINDOWS
    #ifdef RAUM_IMPORT
        #define RAUM_API __declspec(dllimport)
    #else
        #define RAUM_API __declspec(dllexport)
    #endif
#else
    #define RAUM_API
#endif

#include <stddef.h>
#include <string>

namespace raum {
// https://www.reddit.com/r/cpp/comments/phqxiq/transparent_find_is_a_pain_to_get_right/
struct hash_string {
    using is_transparent = void;
    size_t operator()(const std::string& v) const {
        return std::hash<std::string>{}(v);
    }
    size_t operator()(const char* v) const {
        return std::hash<std::string_view>{}(v);
    }

    size_t operator()(const std::string_view& v) const {
        return std::hash<std::string_view>{}(v);
    }
};

template <typename... Args>
requires(!raum_debug) void raum_check(bool, fmt::format_string<Args...> s, Args&&... args) noexcept {}

template <typename... Args>
requires(raum_debug) void raum_check(bool exp, fmt::format_string<Args...> s, Args&&... args) noexcept {
    if (!exp) {
        raum_error(s, std::forward<Args>(args)...);
        assert(false);
    }
}

template <typename... Args>
void raum_expect(bool exp, fmt::format_string<Args...> s, Args&&... args) {
    raum_check(exp, s, args...);
}

inline void raum_unreachable() {
    constexpr auto fmtStr = R"(
--------------------------- UNREACHABLE ---------------------------
file:{}
line:{}
function:{}
)";
    raum_check(false, fmtStr, std::source_location().file_name(), std::source_location().line(), std::source_location().function_name());
}

template<typename ...Args>
using flat_map = boost::container::flat_map<Args...>;

template<typename ...Args>
using flat_set = boost::container::flat_set<Args...>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

} // namespace raum
