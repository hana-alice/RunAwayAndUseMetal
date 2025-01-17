#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSetLayout: public RHIResource  {
public:
    explicit RHIDescriptorSetLayout(const DescriptorSetLayoutInfo& info, RHIDevice*) : _info(info){};

    virtual ~RHIDescriptorSetLayout() = 0;

    const DescriptorSetLayoutInfo& info() const { return _info; }

protected:
    const DescriptorSetLayoutInfo _info;
};

inline RHIDescriptorSetLayout::~RHIDescriptorSetLayout() {}

} // namespace raum::rhi