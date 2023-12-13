#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHISwapchain {
public:
    explicit RHISwapchain(const SwapchainInfo&, RHIDevice*) {}
    virtual ~RHISwapchain() = 0;
};
} // namespace raum::rhi