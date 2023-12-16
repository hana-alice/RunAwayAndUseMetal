#include "VKSwapchain.h"
#ifdef WINDOWS
// clang-format off
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
// clang-format on
#endif
#include "VKDevice.h"
#include "VKQueue.h"
#include "VKUtils.h"
#include "log.h"
#include "VKImageView.h"
#include "VKImage.h"

namespace raum::rhi {
Swapchain::Swapchain(const SwapchainInfo& info, Device* device)
: RHISwapchain(info, device), _device(static_cast<Device*>(device)) {
#ifdef WINDOWS
    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = (HWND)info.hwnd;
    surfaceInfo.hinstance = GetModuleHandle(nullptr);

    auto physicalDevice = device->physicalDevice();
    VkResult res = vkCreateWin32SurfaceKHR(device->instance(), &surfaceInfo, nullptr, &_surface);
    RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create surface");
    VkBool32 support{false};

    auto* grfxQ = device->getQueue({QueueType::GRAPHICS});
    _presentQueue = static_cast<Queue*>(grfxQ);
    auto qIndex = _presentQueue->index();
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, qIndex, _surface, &support);

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

    _preferredFormat = preferred.format;

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
    constexpr uint32_t preferredSwapchainCount = 3;
    if (caps.maxImageCount > 0 && caps.maxImageCount >= preferredSwapchainCount) {
        imageCount = preferredSwapchainCount;
    }

    _presentQueue->initPresentQueue(imageCount);

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
    std::vector<VkImage> scImage(imageCount);
    vkGetSwapchainImagesKHR(device->device(), _swapchain, &imageCount, scImage.data());

    for (auto img : scImage) {
        ImageInfo imageInfo{};
        imageInfo.type = ImageType::IMAGE_2D;
        imageInfo.format = mapSwapchainFormat(_preferredFormat);
        imageInfo.usage = ImageUsage::COLOR_ATTACHMENT | ImageUsage::TRANSFER_DST;
        imageInfo.intialLayout = ImageLayout::UNDEFINED;
        imageInfo.sliceCount = 1;
        imageInfo.mipCount = 1;
        imageInfo.sampleCount = 1;
        imageInfo.extent = {info.width, info.height, 1};
        auto* kImage = _swapchainImages.emplace_back(static_cast<Image*>(new Image(imageInfo, _device, img)));

        ImageViewInfo imageViewInfo{};
        imageViewInfo.format = mapSwapchainFormat(_preferredFormat);
        imageViewInfo.image = kImage;
        imageViewInfo.range = Range{
            AspectMask::COLOR,
            info.width,
            info.height,
            0, 1, 0, 1,
        };
        imageViewInfo.type = ImageViewType::IMAGE_VIEW_2D;
        _swapchainImageViews.emplace_back(static_cast<ImageView*>(_device->createImageView(imageViewInfo)));
    }
#else
    #pragma error Run Away
#endif
}

bool Swapchain::aquire() {
    VkSemaphore presentSemaphore = _presentQueue->presentSemaphore();
    return vkAcquireNextImageKHR(_device->device(), _swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &_imageIndex) == VK_SUCCESS;
}

void Swapchain::present() {
    VkSemaphore renderSemaphore = _presentQueue->renderSemaphore();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &_imageIndex;
    presentInfo.pResults = nullptr;
    vkQueuePresentKHR(_presentQueue->_vkQueue, &presentInfo);
}

Swapchain::~Swapchain() {
    for (auto imgView : _swapchainImageViews) {
        delete imgView;
    }
    vkDestroySwapchainKHR(_device->device(), _swapchain, nullptr);
    vkDestroySurfaceKHR(_device->instance(), _surface, nullptr);
}

RHIImageView* Swapchain::swapchainImageView() const {
    return _swapchainImageViews[_imageIndex];
}

uint32_t Swapchain::imageCount() const {
    return static_cast<uint32_t>(_swapchainImages.size());
}

} // namespace raum::rhi