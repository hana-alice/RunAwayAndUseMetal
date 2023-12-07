#pragma once
#include <vector>
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class Swapchain {
    Swapchain(const SwapchainInfo& info, Device* device);
    ~Swapchain();

    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    std::vector<VkImage> _swapchainImages;

    friend class Device;
};

} // namespace raum::rhi