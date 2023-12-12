#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIGraphicsPipeline {
public:
    explicit RHIGraphicsPipeline(const GraphicsPipelineInfo&){};
    virtual ~RHIGraphicsPipeline() = 0;
};
} // namespace raum::rhi