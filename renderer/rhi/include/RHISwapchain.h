#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHISwapchain {
public:
    explicit RHISwapchain(const SwapchainInfo&) {}
    virtual ~RHISwapchain() = 0;
};
} // namespace raum::rhi