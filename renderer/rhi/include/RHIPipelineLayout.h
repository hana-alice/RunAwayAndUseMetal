#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIPipelineLayout {
public:
    explicit RHIPipelineLayout(const PipelineLayoutInfo&, RHIDevice*) {}

protected:
    virtual ~RHIPipelineLayout() = 0;
};

inline RHIPipelineLayout::~RHIPipelineLayout() {}

} // namespace raum::rhi