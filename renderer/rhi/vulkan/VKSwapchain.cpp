#include "VKSwapchain.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
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

namespace {

struct PendingSwapchain {
    explicit PendingSwapchain(Device* owningDevice) : device(owningDevice) {}

    ~PendingSwapchain() {
        readyPresentSemaphores.clear();
        imageViews.clear();
        images.clear();
        if (swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(device->device(), swapchain, nullptr);
        }
        if (surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(static_cast<VkInstance>(device->instance()), surface, nullptr);
        }
    }

    Device* device;
    VkSurfaceKHR surface{VK_NULL_HANDLE};
    VkSwapchainKHR swapchain{VK_NULL_HANDLE};
    std::vector<VkImage> vkImages;
    std::vector<ImagePtr> images;
    std::vector<ImageViewPtr> imageViews;
    std::vector<std::unique_ptr<Semaphore>> readyPresentSemaphores;
};

} // namespace

void Swapchain::initialize(uintptr_t hwnd, SyncType type, uint32_t width, uint32_t height) {
#ifdef RAUM_WINDOWS
    PendingSwapchain pending{_device};
    auto physicalDevice = _device->physicalDevice();
    auto* grfxQ = _device->getQueue({QueueType::GRAPHICS});
    auto* presentQueue = static_cast<Queue*>(grfxQ);
    auto qIndex = presentQueue->index();

    VkWin32SurfaceCreateInfoKHR surfaceInfo{};
    surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceInfo.hwnd = (HWND)hwnd;
    surfaceInfo.hinstance = GetModuleHandle(nullptr);

    auto instance = static_cast<VkInstance>(_device->instance());
    VK_EXPECT(vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &pending.surface));
    VkBool32 support{false};

    VK_EXPECT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, qIndex, pending.surface, &support));
    VK_ENSURE(support, "The graphics queue cannot present to the Vulkan surface");

    VkSurfaceCapabilitiesKHR caps{};
    VK_EXPECT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, pending.surface, &caps));

    const auto formats = VK_ENUMERATE(
        VkSurfaceFormatKHR, vkGetPhysicalDeviceSurfaceFormatsKHR, physicalDevice, pending.surface);
    VK_ENSURE(!formats.empty(), "The Vulkan surface has no supported formats");

    const auto presentModes = VK_ENUMERATE(
        VkPresentModeKHR, vkGetPhysicalDeviceSurfacePresentModesKHR, physicalDevice, pending.surface);
    VK_ENSURE(!presentModes.empty(), "The Vulkan surface has no supported present modes");

    VkSurfaceFormatKHR preferred = formats[0];
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
        preferred = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }
    for (auto format : formats) {
        if (format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
            format.format == VK_FORMAT_B8G8R8A8_UNORM) {
            preferred = format;
            break;
        }
    }

    const auto preferredFormat = mapSwapchainFormat(preferred.format);

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

    VkExtent2D extent{};
    if (caps.currentExtent.width != UINT32_MAX) {
        extent = caps.currentExtent;
    } else {
        extent.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0) {
        imageCount = (std::min)(imageCount, caps.maxImageCount);
    }

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    constexpr std::array<VkCompositeAlphaFlagBitsKHR, 4> compositeAlphaOptions{
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (const auto option : compositeAlphaOptions) {
        if (caps.supportedCompositeAlpha & option) {
            compositeAlpha = option;
            break;
        }
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = pending.surface;
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
    createInfo.compositeAlpha = compositeAlpha;
    createInfo.presentMode = mode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VK_EXPECT(vkCreateSwapchainKHR(_device->device(), &createInfo, nullptr, &pending.swapchain));

    pending.vkImages = VK_ENUMERATE(VkImage, vkGetSwapchainImagesKHR, _device->device(), pending.swapchain);
    VK_ENSURE(!pending.vkImages.empty(), "The Vulkan swapchain has no images");
    imageCount = static_cast<uint32_t>(pending.vkImages.size());

#else
    #pragma error Run Away
#endif

    std::vector<Semaphore*> readyPresentSemaphores;
    readyPresentSemaphores.reserve(imageCount);
    pending.readyPresentSemaphores.reserve(imageCount);
    for (size_t i = 0; i < imageCount; ++i) {
        pending.readyPresentSemaphores.emplace_back(
            static_cast<Semaphore*>(_device->createSemaphore()));
        readyPresentSemaphores.emplace_back(pending.readyPresentSemaphores.back().get());
    }

    std::vector<Semaphore*> acquireSemaphores(imageCount, nullptr);
    std::vector<uint32_t> valid(imageCount, 1);

    pending.images.reserve(imageCount);
    pending.imageViews.reserve(imageCount);
    for (const auto vkImage : pending.vkImages) {
        ImageInfo imageInfo{};
        imageInfo.type = ImageType::IMAGE_2D;
        imageInfo.format = preferredFormat;
        imageInfo.usage = ImageUsage::COLOR_ATTACHMENT | ImageUsage::TRANSFER_DST;
        imageInfo.initialLayout = ImageLayout::UNDEFINED;
        imageInfo.sliceCount = 1;
        imageInfo.mipCount = 1;
        imageInfo.sampleCount = 1;
        imageInfo.extent = {extent.width, extent.height, 1};
        auto image = ImagePtr(new Image(imageInfo, _device, vkImage));

        ImageViewInfo viewInfo{};
        viewInfo.type = ImageViewType::IMAGE_VIEW_2D;
        viewInfo.image = image.get();
        viewInfo.format = preferredFormat;
        viewInfo.range = {
            .aspect = AspectMask::COLOR,
            .sliceCount = 1,
            .mipCount = 1,
        };
        pending.imageViews.emplace_back(_device->createImageView(viewInfo));
        pending.images.emplace_back(std::move(image));
    }

    _surface = std::exchange(pending.surface, VK_NULL_HANDLE);
    _swapchain = std::exchange(pending.swapchain, VK_NULL_HANDLE);
    _vkImages = std::move(pending.vkImages);
    _images = std::move(pending.images);
    _imageViews = std::move(pending.imageViews);
    _acquireSemaphores = std::move(acquireSemaphores);
    _readyPresentSemaphores = std::move(readyPresentSemaphores);
    for (auto& semaphore : pending.readyPresentSemaphores) {
        semaphore.release();
    }
    _valid = std::move(valid);
    _imageCount = imageCount;
    _imageIndex = 0;
    _preferredFormat = preferredFormat;
    _presentQueue = presentQueue;
    _info.width = extent.width;
    _info.height = extent.height;
    _info.type = type;
    _info.hwnd = hwnd;
}

Swapchain::Swapchain(const SwapchainInfo& info, Device* device)
: RHISwapchain(info, device), _device(static_cast<Device*>(device)), _info(info) {
    initialize(info.hwnd, info.type, info.width, info.height);
}

Swapchain::Swapchain(const raum::rhi::SwapchainSurfaceInfo& info, raum::rhi::Device* device)
: RHISwapchain(info, device), _device(static_cast<Device*>(device)) {
    _info = {info.width, info.height, info.type, info.windId};
    initialize(info.windId, info.type, info.width, info.height);
}

RHISemaphore* Swapchain::getAvailableSemaphore() {
    return _acquireSemaphores[_imageIndex];
}

RHISemaphore* Swapchain::getSignalPresentSemaphore() {
    return _readyPresentSemaphores[_imageIndex];
}

bool Swapchain::acquire() {
    Semaphore* acquireSem = _semaphorePool.allocate(_device);
    const auto result = vkAcquireNextImageKHR(
        _device->device(), _swapchain, UINT64_MAX, acquireSem->semaphore(), VK_NULL_HANDLE, &_imageIndex);
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        _semaphorePool.dealloccate(acquireSem);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            return false;
        }
    }
    VK_CHECK(result, VK_SUCCESS, VK_SUBOPTIMAL_KHR);
    if (_acquireSemaphores[_imageIndex]) [[likely]] {
        _semaphorePool.dealloccate(_acquireSemaphores[_imageIndex]);
    }
    _acquireSemaphores[_imageIndex] = acquireSem;
    return true;
}

void Swapchain::present() {
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    auto sem = _readyPresentSemaphores[_imageIndex]->semaphore();
    presentInfo.pWaitSemaphores = &sem;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.pImageIndices = &_imageIndex;
    presentInfo.pResults = nullptr;
    const auto result = vkQueuePresentKHR(_presentQueue->_vkQueue, &presentInfo);
    _presentQueue->increaseFrameIndex();
    VK_CHECK(result, VK_SUCCESS, VK_SUBOPTIMAL_KHR, VK_ERROR_OUT_OF_DATE_KHR);
}

void Swapchain::destroy() {
    if (_presentQueue && _presentQueue->_vkQueue != VK_NULL_HANDLE) {
        vkQueueWaitIdle(_presentQueue->_vkQueue);
    }

    for (auto*& sem : _acquireSemaphores) {
        delete sem;
        sem = nullptr;
    }
    _acquireSemaphores.clear();

    for (auto*& sem : _readyPresentSemaphores) {
        delete sem;
        sem = nullptr;
    }
    _readyPresentSemaphores.clear();

    _imageViews.clear();
    _images.clear();

    auto instance = static_cast<VkInstance>(_device->instance());
    if (_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(_device->device(), _swapchain, nullptr);
        _swapchain = VK_NULL_HANDLE;
    }
    if (_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance, _surface, nullptr);
        _surface = VK_NULL_HANDLE;
    }
    _vkImages.clear();
    _valid.clear();
    _imageCount = 0;
}

Swapchain::~Swapchain() {
    destroy();
}

bool Swapchain::imageValid(uint32_t index) {
    return index < _valid.size() && _valid[index];
}

bool Swapchain::holds(RHIImage* img) {
    return std::ranges::any_of(_images, [img](const auto& image) {
        return image.get() == img;
    });
}

ImagePtr Swapchain::image(uint32_t index) {
    if (index >= _images.size()) {
        raum_error("Vulkan swapchain image index {} is out of range", index);
    }
    return _images[index];
}

ImageViewPtr Swapchain::imageView(uint32_t index) {
    if (index >= _imageViews.size()) {
        raum_error("Vulkan swapchain image-view index {} is out of range", index);
    }
    return _imageViews[index];
}

uint32_t Swapchain::imageCount() const {
    return static_cast<uint32_t>(_vkImages.size());
}

uint32_t Swapchain::imageIndex() const {
    return _imageIndex;
}

void Swapchain::resize(uint32_t w, uint32_t h) {
    if (!w || !h || (w == _info.width && h == _info.height)) {
        return;
    }
    destroy();
    _info.width = w;
    _info.height = h;
    initialize(_info.hwnd, _info.type, w, h);
}

void Swapchain::resize(uint32_t w, uint32_t h, uintptr_t surface) {
    if (!w || !h || (w == _info.width && h == _info.height && surface == _info.hwnd)) {
        return;
    }
    destroy();
    _info.width = w;
    _info.height = h;
    _info.hwnd = surface;
    initialize(surface, _info.type, w, h);
}

} // namespace raum::rhi
