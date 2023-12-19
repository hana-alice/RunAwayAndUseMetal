#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSet {
public:
    explicit RHIDescriptorSet(const DescriptorSetInfo&, RHIDevice*) {}

    virtual ~RHIDescriptorSet() = 0;

protected:
};

inline RHIDescriptorSet::~RHIDescriptorSet() {}

} // namespace raum::rhi