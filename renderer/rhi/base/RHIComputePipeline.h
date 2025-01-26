#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"

namespace raum::rhi {
class RHIDevice;
class RHIComputePipeline: public RHIResource {
public:
    explicit RHIComputePipeline(const ComputePipelineInfo&, RHIDevice*) {};
    virtual ~RHIComputePipeline() = 0;
};

inline RHIComputePipeline::~RHIComputePipeline() {}

} // namespace raum::rhi