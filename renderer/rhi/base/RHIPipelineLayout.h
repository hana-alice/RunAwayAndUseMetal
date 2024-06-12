#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIPipelineLayout : public RHIResource {
public:
    explicit RHIPipelineLayout(const PipelineLayoutInfo& info, RHIDevice* device): _info(info) {}

    virtual ~RHIPipelineLayout() = 0;

    const PipelineLayoutInfo& info() { return _info; }
protected:
    PipelineLayoutInfo _info;
};

inline RHIPipelineLayout::~RHIPipelineLayout() {}

} // namespace raum::rhi