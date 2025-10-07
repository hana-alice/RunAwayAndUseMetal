#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIRaytracingPipeline: public RHIResource  {
public:
    explicit RHIRaytracingPipeline(const RaytracingPipelineInfo&, RHIDevice*){};

    virtual ~RHIRaytracingPipeline() = 0;

protected:
};

inline RHIRaytracingPipeline::~RHIRaytracingPipeline() {}

} // namespace raum::rhi