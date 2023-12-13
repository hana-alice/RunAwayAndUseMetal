#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIPipelineLayout {
public:
    explicit RHIPipelineLayout(const PipelineLayoutInfo&, RHIDevice*) {}
    virtual ~RHIPipelineLayout() = 0;
};
} // namespace raum::rhi