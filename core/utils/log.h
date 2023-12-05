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

#define RAUM_ERROR(con, id)                     \
    {                                           \
        if (con) spdlog::error("error {}", id); \
    }

namespace raum {

template <typename T>
void log(const T& t);

void log(const char* msg) {
    spdlog::log(spdlog::level::info, msg);
}

} // namespace raum