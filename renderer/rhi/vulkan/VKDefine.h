#pragma once
#include <vulkan/vulkan.h>
#include "RHIDefine.h"
namespace raum::rhi {
#define VK_CHECK_RESULT(f)                                                 \
    {                                                                      \
        VkResult res = (f);                                                \
        if (res != VK_SUCCESS) {                                           \
            raum_error("VkResult is {:08X}.", static_cast<uint32_t>(res)); \
            throw std::runtime_error("VkResult is not VK_SUCCESS.");       \
        }                                                                  \
    }

struct FormatInfo {
    VkFormat format;
    uint32_t size;
    uint32_t macroPixelCount;
};

} // namespace raum::rhi