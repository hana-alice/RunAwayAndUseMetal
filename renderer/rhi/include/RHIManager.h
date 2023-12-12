#pragma once
#include "RHIDevice.h"
namespace raum::rhi {

enum class API : unsigned char {
    VULKAN,
};

inline RHIDevice* loadRHI(API api);
inline void unloadRHI(RHIDevice* device);

} // namespace raum::rhi