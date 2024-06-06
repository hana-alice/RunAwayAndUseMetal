#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIRenderPass: public RHIResource  {
public:
    explicit RHIRenderPass(const RenderPassInfo& info, RHIDevice*) : _info(info) {}

    inline const std::vector<AttachmentInfo>& attachments() const {
        return _info.attachments;
    }

    const RenderPassInfo& info() const { return _info; }

    virtual ~RHIRenderPass() = 0;

protected:
    const RenderPassInfo _info;
};

inline RHIRenderPass::~RHIRenderPass() {}

} // namespace raum::rhi