#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIRenderPass {
public:
    explicit RHIRenderPass(const RenderPassInfo&) {}
    virtual ~RHIRenderPass() = 0;
};
} // namespace raum::rhi