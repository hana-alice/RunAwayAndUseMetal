#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIGraphicsPipeline {
public:
    explicit RHIGraphicsPipeline(const GraphicsPipelineInfo&, RHIDevice*){};

protected:
    virtual ~RHIGraphicsPipeline() = 0;
};

inline RHIGraphicsPipeline::~RHIGraphicsPipeline() {}

} // namespace raum::rhi