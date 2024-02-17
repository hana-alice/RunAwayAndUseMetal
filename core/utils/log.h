#pragma once

#include <ostream>
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

#define RAUM_ERROR_IF(con, ...)              \
    {                                        \
        if (con) spdlog::error(__VA_ARGS__); \
    }

#define RAUM_ERROR(...)             \
    {                               \
        spdlog::error(__VA_ARGS__); \
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

template <typename T>
void log(const T& t) {
    spdlog::log(spdlog::level::info, t);
}

template<typename ...Args>
void error(fmt::format_string<Args...> s, Args&&... args) {
    spdlog::error(s, std::forward<Args>(args)...);
}

} // namespace raum