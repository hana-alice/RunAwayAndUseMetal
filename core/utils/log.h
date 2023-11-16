#pragma once

#include "spdlog/spdlog.h"

#define RAUM_CRITICAL_IF(con, ...)               \
    {                                            \
        if (!con) spdlog::critical(__VA_ARGS__); \
    }