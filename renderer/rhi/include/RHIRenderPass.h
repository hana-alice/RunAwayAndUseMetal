#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIRenderPass {
public:
    explicit RHIRenderPass(const RenderPassInfo& info, RHIDevice*) : _info(info) {}

    inline const std::vector<AttachmentInfo>& attachments() const {
        return _info.attachments;
    }

protected:
    const RenderPassInfo _info;
    virtual ~RHIRenderPass() = 0;
};

inline RHIRenderPass::~RHIRenderPass() {}

} // namespace raum::rhi