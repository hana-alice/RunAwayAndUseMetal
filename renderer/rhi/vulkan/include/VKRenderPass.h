#pragma once
#include "VKDefine.h"

namespace raum::rhi {
class RenderPass {
public:
    RenderPass() = delete;
    RenderPass(const RenderPass&) = delete;
    RenderPass(RenderPass&&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;

    explicit RenderPass(const RenderPassInfo& info);
    ~RenderPass();

    VkRenderPass renderPass() const { return _renderPass; }

private:
    VkRenderPass _renderPass;
};
} // namespace raum::rhi