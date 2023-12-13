#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSetLayout {
public:
    explicit RHIDescriptorSetLayout(const DescriptorSetLayoutInfo&, RHIDevice*){};

protected:
    virtual ~RHIDescriptorSetLayout() = 0;
};

inline RHIDescriptorSetLayout::~RHIDescriptorSetLayout() {}

} // namespace raum::rhi