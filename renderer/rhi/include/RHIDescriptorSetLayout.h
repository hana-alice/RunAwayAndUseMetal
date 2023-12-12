#pragma once
#include "RHIDefine.h"
namespace raum::rhi {

class RHIDescriptorSetLayout {
public:
    explicit RHIDescriptorSetLayout(const DescriptorSetLayoutInfo&){};
    virtual ~RHIDescriptorSetLayout() = 0;
};
} // namespace raum::rhi