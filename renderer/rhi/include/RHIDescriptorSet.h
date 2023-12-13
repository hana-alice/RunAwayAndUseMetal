#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSet {
public:
    explicit RHIDescriptorSet(const DescriptorSetInfo&, RHIDevice*) {}

protected:
    virtual ~RHIDescriptorSet() = 0;
};

inline RHIDescriptorSet::~RHIDescriptorSet() {}

} // namespace raum::rhi