#pragma once

#include <exception>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include "lut.h"
#include "spdlog/spdlog.h"

#define RAUM_CRITICAL_IF(con, ...)              \
    {                                           \
        if (con) spdlog::critical(__VA_ARGS__); \
    }

#define RAUM_CRITICAL(con, id)                         \
    {                                                  \
        if (con) spdlog::critical(criticalLUT.at(id)); \
    }

#define RAUM_WARN_IF(con, ...)              \
    {                                       \
        if (con) spdlog::warn(__VA_ARGS__); \
    }

#define RAUM_WARN(...)             \
    {                              \
        spdlog::warn(__VA_ARGS__); \
    }

namespace raum {

#ifndef RAUM_ERROR_THROWS
    #define RAUM_ERROR_THROWS 1
#endif

inline constexpr bool raum_error_throws{RAUM_ERROR_THROWS != 0};

template <typename T>
void log(const T& t) {
    spdlog::log(spdlog::level::info, t);
}

template <typename... Args>
void raum_report_error(fmt::format_string<Args...> s, Args&&... args) noexcept {
    try {
        spdlog::error(s, std::forward<Args>(args)...);
    } catch (...) {
        // Reporting is also used at C ABI boundaries and must never propagate.
    }
}

template <typename... Args>
[[noreturn]] void raum_error(fmt::format_string<Args...> s, Args&&... args) {
    if constexpr (raum_error_throws) {
        auto message = fmt::format(s, std::forward<Args>(args)...);
        raum_report_error("{}", message);
        throw std::runtime_error(std::move(message));
    } else {
        raum_report_error(s, std::forward<Args>(args)...);
        std::terminate();
    }
}

template<typename ...Args>
void raum_warn(fmt::format_string<Args...> s, Args&&... args) {
    spdlog::warn(s, std::forward<Args>(args)...);
}

template<typename ...Args>
void raum_info(fmt::format_string<Args...> s, Args&&... args) {
    spdlog::info(s, std::forward<Args>(args)...);
}

} // namespace raum
