#pragma once
#include <unordered_map>
#include <stdint.h>

typedef uint32_t LogID;
static std::unordered_map<LogID, const char*> criticalLUT = {
    {1, "1"},
};