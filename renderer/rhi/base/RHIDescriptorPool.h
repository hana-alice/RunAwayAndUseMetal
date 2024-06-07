#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"

namespace raum::rhi {
class RHIDescriptorSet;
class RHIDescriptorSetLayout;
class RHIDevice;
class RHIDescriptorPool: public RHIResource  {
public:
    explicit RHIDescriptorPool(const DescriptorPoolInfo&, RHIDevice* device) {}
    virtual ~RHIDescriptorPool() {}

    virtual RHIDescriptorSet* makeDescriptorSet(const DescriptorSetInfo&) = 0;
};
}