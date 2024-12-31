#include "VKSwapchain.h"
#ifdef RAUM_WINDOWS
// clang-format off
    #include <windows.h>
    #include <vulkan/vulkan_win32.h>
// clang-format on
#endif
#include "VKDevice.h"
#include "VKImage.h"
#include "VKImageView.h"
#include "VKQueue.h"
#include "VKSemaphore.h"
#include "VKUtils.h"

namespace raum::rhi {

void Swapchain::initialize(uintptr_t hwnd, SyncType type, uint32_t width, uint32_t height) {
#ifdef RAUM_WINDOWS
    auto physicalDevice = _device->physicalDevice();
    auto* grfxQ = _device->getQueue({QueueType::GRAPHICS});
    _presentQueue = static_cast<Queue*>(grfxQ);
    auto qIndex = _presentQueue->index();

    if (_surface == VK_NULL_HANDLE) {
        VkWin32SurfaceCreateInfoKHR surfaceInfo{};
        surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surfaceInfo.hwnd = (HWND)hwnd;
        surfaceInfo.hinstance = GetModuleHandle(nullptr);

        auto instance = static_cast<VkInstance>(_device->instance());
        VkResult res = vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &_surface);
        RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create surface");
        VkBool32 support{false};

        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, qIndex, _surface, &support);
        RAUM_CRITICAL_IF(!support, "surface presentation not supported");
    }

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
            format.format == VK_FORMAT_B8G8R8_SNORM) {
            preferred = format;
            break;
        }
    }

    _preferredFormat = mapSwapchainFormat(preferred.format);

    VkPresentModeKHR mode{VK_PRESENT_MODE_FIFO_KHR};
    VkPresentModeKHR hint{VK_PRESENT_MODE_FIFO_KHR};
    switch (type) {
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

    VkExtent2D extent{width, height};

    uint32_t imageCount = caps.minImageCount;
    constexpr uint32_t preferredSwapchainCount = 3;
    if (caps.maxImageCount > 0 && caps.maxImageCount >= preferredSwapchainCount) {
        imageCount = preferredSwapchainCount;
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

    auto res = vkCreateSwapchainKHR(_device->device(), &createInfo, nullptr, &_swapchain);
    RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create swapchain");

    vkGetSwapchainImagesKHR(_device->device(), _swapchain, &imageCount, nullptr);
    _vkImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device->device(), _swapchain, &imageCount, _vkImages.data());

    _valid.clear();
    _valid.resize(imageCount, 0);

#else
    #pragma error Run Away
#endif

    _acquireSemaphores.resize(imageCount);
    for (auto& sem : _acquireSemaphores) {
        sem = new Semaphore(_device);
    }
}

Swapchain::Swapchain(const SwapchainInfo& info, Device* device)
: RHISwapchain(info, device), _device(static_cast<Device*>(device)), _info(info) {
    initialize(info.hwnd, info.type, info.width, info.height);
}

Swapchain::Swapchain(const raum::rhi::SwapchainSurfaceInfo& info, raum::rhi::Device* device)
: RHISwapchain(info, device), _device(static_cast<Device*>(device)) {
    _info = {info.width, info.height, info.type, 0};
    initialize(info.windId, info.type, info.width, info.height);
}

void Swapchain::addWaitBeforePresent(RHISemaphore* s) {
    auto* sem = static_cast<Semaphore*>(s);
    _waits.emplace_back(sem);
}

RHISemaphore* Swapchain::getAvailableByAcquire() {
    return _acquireSemaphores[_imageIndex];
}

bool Swapchain::acquire() {
    auto imageAvailableSem = _acquireSemaphores[_imageIndex];
    return vkAcquireNextImageKHR(_device->device(), _swapchain, UINT64_MAX, imageAvailableSem->semaphore(), VK_NULL_HANDLE, &_imageIndex) == VK_SUCCESS;
}

void Swapchain::present() {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    std::vector<VkSemaphore> sems{};
    if (!_waits.empty()) [[likely]] {
        for (auto sem : _waits) {
            sems.emplace_back(sem->semaphore());
        }
        presentInfo.pWaitSemaphores = sems.data();
        presentInfo.waitSemaphoreCount = sems.size();
        _waits.clear();
    } else {
        presentInfo.waitSemaphoreCount = 0;
        presentInfo.pWaitSemaphores = nullptr;
    }

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &_imageIndex;
    presentInfo.pResults = nullptr;
    vkQueuePresentKHR(_presentQueue->_vkQueue, &presentInfo);
}

void Swapchain::destroy() {
    vkQueueWaitIdle(_presentQueue->_vkQueue);

    auto instance = static_cast<VkInstance>(_device->instance());
    vkDestroySurfaceKHR(instance, _surface, nullptr);
    vkDestroySwapchainKHR(_device->device(), _swapchain, nullptr);
}

Swapchain::~Swapchain() {
    destroy();
}

bool Swapchain::imageValid(uint32_t index) {
    return _valid[index];
}

bool Swapchain::holds(RHIImage* img) {
    return static_cast<Image*>(img)->_swapchain;
}

RHIImage* Swapchain::allocateImage(uint32_t index) {
    _valid[index] = 1;
    auto img = _vkImages[index];

    ImageInfo imageInfo{};
    imageInfo.type = ImageType::IMAGE_2D;
    imageInfo.format = _preferredFormat;
    imageInfo.usage = ImageUsage::COLOR_ATTACHMENT | ImageUsage::TRANSFER_DST;
    imageInfo.initialLayout = ImageLayout::UNDEFINED;
    imageInfo.sliceCount = 1;
    imageInfo.mipCount = 1;
    imageInfo.sampleCount = 1;
    imageInfo.extent = {_info.width, _info.height, 1};
    return new Image(imageInfo, _device, img);
}

uint32_t Swapchain::imageCount() const {
    return static_cast<uint32_t>(_vkImages.size());
}

uint32_t Swapchain::imageIndex() const {
    return _imageIndex;
}

void Swapchain::resize(uint32_t w, uint32_t h) {
    destroy();
    _info.width = w;
    _info.height = h;
    initialize(_info.hwnd, _info.type, w, h);
}

void Swapchain::resize(uint32_t w, uint32_t h, uintptr_t surface) {
    if (w == _info.width && h == _info.height) {
        return;
    }
    destroy();
    vkQueueWaitIdle(_presentQueue->_vkQueue);
    vkDestroySwapchainKHR(_device->device(), _swapchain, nullptr);
    _info.width = w;
    _info.height = h;
    initialize(surface, _info.type, w, h);
}

} // namespace raum::rhi