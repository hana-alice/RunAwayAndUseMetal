#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIGraphicsPipeline {
public:
    explicit RHIGraphicsPipeline(const GraphicsPipelineInfo&, RHIDevice*){};
    virtual ~RHIGraphicsPipeline() = 0;
};
} // namespace raum::rhi