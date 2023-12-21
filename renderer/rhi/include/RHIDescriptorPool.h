#pragma once
#include "RHIDefine.h"

namespace raum::rhi {
class RHIDescriptorSet;
class RHIDescriptorSetLayout;
class RHIDevice;
class RHIDescriptorPool {
public:
    explicit RHIDescriptorPool(const DescriptorPoolInfo&, RHIDevice* device) {}
    virtual ~RHIDescriptorPool() {}

    virtual RHIDescriptorSet* makeDescriptorSet(const DescriptorSetInfo&) = 0;
};
}