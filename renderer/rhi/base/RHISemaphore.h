#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHISemaphore : public RHIResource {
public:
    explicit RHISemaphore(RHIDevice*) {}

    virtual void setStage(rhi::PipelineStage stage) = 0;

    virtual PipelineStage getStage() = 0;

    virtual ~RHISemaphore() = 0;
};

inline RHISemaphore::~RHISemaphore() {}

} // namespace raum::rhi