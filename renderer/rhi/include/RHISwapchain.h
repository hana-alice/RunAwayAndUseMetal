#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHISwapchain {
public:
    explicit RHISwapchain(const SwapchainInfo&, RHIDevice*) {}

protected:
    virtual ~RHISwapchain() = 0;
};

inline RHISwapchain::~RHISwapchain() {}

} // namespace raum::rhi