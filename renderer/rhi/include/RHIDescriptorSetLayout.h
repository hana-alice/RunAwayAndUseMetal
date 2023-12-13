#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSetLayout {
public:
    explicit RHIDescriptorSetLayout(const DescriptorSetLayoutInfo&, RHIDevice*){};
    virtual ~RHIDescriptorSetLayout() = 0;
};
} // namespace raum::rhi