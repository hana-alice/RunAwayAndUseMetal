#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIPipelineLayout {
public:
    explicit RHIPipelineLayout(const PipelineLayoutInfo&) {}
    virtual ~RHIPipelineLayout() = 0;
};
} // namespace raum::rhi