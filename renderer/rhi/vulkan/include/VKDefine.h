#pragma once
#include <vulkan/vulkan.h>
#include "RHIDefine.h"
namespace raum::rhi {
struct FormatInfo {
    VkFormat format;
    uint32_t size;
    uint32_t macroPixelCount;
};
} // namespace raum::rhi