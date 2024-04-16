#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIImageView;
class RHISwapchain {
public:
    explicit RHISwapchain(const SwapchainInfo&, RHIDevice*) {}

    virtual bool aquire() = 0;
    virtual void present() = 0;

    virtual uint32_t imageCount() const = 0;
    virtual uint32_t imageIndex() const = 0;
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual Format format() const = 0;

    virtual RHIImage* allocateImage(uint32_t index) = 0;
    virtual ~RHISwapchain() = 0;
};

inline RHISwapchain::~RHISwapchain() {}

} // namespace raum::rhi