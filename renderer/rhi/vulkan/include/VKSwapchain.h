#pragma once
#include <vector>
#include "RHISwapchain.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class ImageView;
class Image;
class Queue;
class Swapchain : public RHISwapchain {
public:
    bool acquire() override;
    void present() override;
    RHIImage* allocateImage(uint32_t index) override;
    uint32_t imageCount() const override;
    uint32_t imageIndex() const override;
    uint32_t width() const override { return _info.width; }
    uint32_t height() const override { return _info.height; }
    Format format() const override { return _preferredFormat; }

    void resize(uint32_t w, uint32_t h) override;
    void resize(uint32_t w, uint32_t h, void* surface) override;

    bool imageValid(uint32_t index) override;
    bool holds(RHIImage* img) override;

private:
    Swapchain(const SwapchainSurfaceInfo& info, Device* device);
    Swapchain(const SwapchainInfo& info, Device* device);

    void destroy();

    ~Swapchain();

    void initialize(uintptr_t hwnd, SyncType type, uint32_t width, uint32_t height);

    SwapchainInfo _info;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    Format _preferredFormat;
    Device* _device{nullptr};
    Queue* _presentQueue{nullptr};

    uint32_t _imageIndex{0};
    std::vector<VkImage> _vkImages;
    std::vector<VkSemaphore> _presentSemaphores;
    std::vector<uint32_t> _valid;

    friend class Device;
};

} // namespace raum::rhi