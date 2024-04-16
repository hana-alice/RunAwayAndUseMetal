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
    bool aquire() override;
    void present() override;
    RHIImage* allocateImage(uint32_t index) override;
    uint32_t imageCount() const override;
    uint32_t imageIndex() const override;
    uint32_t width() const override { return _info.width; }
    uint32_t height() const override { return _info.height; }
    Format format() const override { return _preferredFormat; }

private:
    Swapchain(const SwapchainInfo& info, Device* device);
    ~Swapchain();

    SwapchainInfo _info;
    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    Format _preferredFormat;
    Device* _device{nullptr};
    Queue* _presentQueue{nullptr};

    uint32_t _imageIndex{0};
    std::vector<VkImage> _vkImages;

    friend class Device;
};

} // namespace raum::rhi