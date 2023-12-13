#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIRenderPass {
public:
    explicit RHIRenderPass(const RenderPassInfo&, RHIDevice*) {}

protected:
    virtual ~RHIRenderPass() = 0;
};

inline RHIRenderPass::~RHIRenderPass() {}

} // namespace raum::rhi