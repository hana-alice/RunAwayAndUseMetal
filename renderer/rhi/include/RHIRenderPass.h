#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIRenderPass {
public:
    explicit RHIRenderPass(const RenderPassInfo&, RHIDevice*) {}
    virtual ~RHIRenderPass() = 0;
};
} // namespace raum::rhi