#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"

namespace raum::rhi {

class RHICommandBuffer;
class RHIDevice;
class RHICommandPool: public RHIResource  {
public:
    explicit RHICommandPool(const CommandPoolInfo&, RHIDevice* device) {}
    virtual ~RHICommandPool() {}

    virtual RHICommandBuffer* makeCommandBuffer(const CommandBufferInfo&) = 0;
};

} // namespace raum::rhi