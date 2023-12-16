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
    RHIImageView* swapchainImageView() const override;

private:
    Swapchain(const SwapchainInfo& info, Device* device);
    ~Swapchain();

    VkSurfaceKHR _surface;
    VkSwapchainKHR _swapchain;
    std::vector<Image*> _swapchainImages;
    std::vector<ImageView*> _swapchainImageViews;
    VkFormat _preferredFormat;
    Device* _device{nullptr};
    Queue* _presentQueue{nullptr};

    uint32_t _imageIndex{0};

    friend class Device;
};

} // namespace raum::rhi