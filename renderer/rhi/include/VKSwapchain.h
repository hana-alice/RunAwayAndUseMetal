#pragma once
#include "VKDefine.h"

namespace raum::rhi {
class Device;
class Swapchain {
    Swapchain(const SwapchainInfo& info, Device* device);
    ~Swapchain();

    VkSurfaceKHR _surface;

    friend class Device;
};

}