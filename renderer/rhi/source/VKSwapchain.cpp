#include "VKSwapchain.h"
#ifdef WINDOWS
// clang-format off
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
// clang-format on
#endif
#include "VKDevice.h"
#include "VKQueue.h"
#include "log.h"

namespace raum::rhi {
Swapchain::Swapchain(const SwapchainInfo& info, Device* device) {
#ifdef WINDOWS
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = (HWND)info.hwnd;
    surfaceInfo.hinstance = GetModuleHandle(nullptr);

    auto physicalDevice = device->physicalDevice();
    VkResult res = vkCreateWin32SurfaceKHR(device->instance(), &surfaceInfo, nullptr, &_surface);
    RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create surface");
    VkBool32 support{false};
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, device->defaultQueue()->index(), _surface, &support);

    RAUM_CRITICAL_IF(!support, "surface presentation not supported");

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, _surface, &caps);

    uint32_t formatCount{0};
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, _surface, &formatCount, formats.data());

    uint32_t presentModeCount{0};
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, nullptr);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, _surface, &presentModeCount, presentModes.data());

    RAUM_CRITICAL_IF(!formatCount || !presentModeCount, "no available format or present mode");

    VkSurfaceFormatKHR preferred = formats[0];
    for (auto format : formats) {
        if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
            format.colorSpace == VK_FORMAT_B8G8R8_SNORM) {
            preferred = format;
            break;
        }
    }

    VkPresentModeKHR mode{VK_PRESENT_MODE_FIFO_KHR};
    VkPresentModeKHR hint{VK_PRESENT_MODE_FIFO_KHR};
    switch (info.type) {
        case SyncType::IMMEDIATE:
            hint = VK_PRESENT_MODE_IMMEDIATE_KHR;
            break;
        case SyncType::VSYNC:
            hint = VK_PRESENT_MODE_FIFO_KHR;
            break;
        case SyncType::RELAX:
            hint = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
            break;
        case SyncType::MAILBOX:
            hint = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
    }
    for (auto presentMode : presentModes) {
        if (presentMode == hint) {
            mode = hint;
        }
    }

    VkExtent2D extent{info.width, info.height};

    uint32_t imageCount = caps.minImageCount;
    if (caps.maxImageCount > 0 && caps.maxImageCount >= SwapchainCount) {
        imageCount = SwapchainCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = _surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = preferred.format;
    createInfo.imageColorSpace = preferred.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = nullptr;
    createInfo.preTransform = caps.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    res = vkCreateSwapchainKHR(device->device(), &createInfo, nullptr, &_swapchain);
    RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create swapchain");

    vkGetSwapchainImagesKHR(device->device(), _swapchain, &imageCount, nullptr);
    _swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device->device(), _swapchain, &imageCount, _swapchainImages.data());

#else
    #pragma error Run Away
#endif
}

Swapchain::~Swapchain() {
    vkDestroySwapchainKHR(Device::getInstance()->device(), _swapchain, nullptr);
    vkDestroySurfaceKHR(Device::getInstance()->instance(), _surface, nullptr);
}
} // namespace raum::rhi