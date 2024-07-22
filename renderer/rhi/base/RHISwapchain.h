#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIImageView;
class RHISwapchain : public RHIResource {
public:
    explicit RHISwapchain(const SwapchainInfo&, RHIDevice*) {};
    explicit RHISwapchain(const SwapchainSurfaceInfo&, RHIDevice*) {};

    virtual bool acquire() = 0;
    virtual void present() = 0;

    virtual uint32_t imageCount() const = 0;
    virtual uint32_t imageIndex() const = 0;
    virtual uint32_t width() const = 0;
    virtual uint32_t height() const = 0;
    virtual Format format() const = 0;
    virtual void resize(uint32_t w, uint32_t h) = 0;
    virtual void resize(uint32_t w, uint32_t h, void* surface) = 0;

    virtual RHIImage* allocateImage(uint32_t index) = 0;
    virtual bool imageValid(uint32_t index) = 0;
    virtual bool holds(RHIImage* img) = 0;
    virtual void addWaitBeforePresent(RHISemaphore* sem) = 0;
    virtual RHISemaphore* getAvailableByAcquire() = 0;
    virtual ~RHISwapchain() = 0;
};

inline RHISwapchain::~RHISwapchain() {}

} // namespace raum::rhi