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

    virtual ~RHIRenderPass() = 0;

protected:
    const RenderPassInfo _info;
};

inline RHIRenderPass::~RHIRenderPass() {}

} // namespace raum::rhi