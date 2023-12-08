#pragma once
#include <vector>
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class ImageView;
class Swapchain {
public:
private:
    Swapchain(const SwapchainInfo& info, Device* device);
    ~Swapchain();

    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;

    VkFormat _preferredFormat;

    friend class Device;
};

} // namespace raum::rhi