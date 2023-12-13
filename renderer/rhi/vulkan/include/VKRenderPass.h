#pragma once
#include "RHIRenderPass.h"
#include "VKDefine.h"

namespace raum::rhi {
class Device;
class RenderPass : public RHIRenderPass {
public:
    RenderPass() = delete;
    RenderPass(const RenderPass&) = delete;
    RenderPass(RenderPass&&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    explicit RenderPass(const RenderPassInfo& info, RHIDevice* device);
    ~RenderPass();

    VkRenderPass renderPass() const { return _renderPass; }

private:
    Device* _device{nullptr};
    VkRenderPass _renderPass;
};
} // namespace raum::rhi