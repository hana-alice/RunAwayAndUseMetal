#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSet: public RHIResource  {
public:
    explicit RHIDescriptorSet(const DescriptorSetInfo&, RHIDevice*) {}

    virtual ~RHIDescriptorSet() = 0;

    virtual void update(const BindingInfo& info) = 0;

    virtual void updateBuffer(const BufferBinding& info) = 0;

    virtual void updateImage(const ImageBinding& info) = 0;

    virtual void updateSampler(const SamplerBinding& info) = 0;

    virtual void updateTexelBuffer(const TexelBufferBinding& info) = 0;

protected:
};

inline RHIDescriptorSet::~RHIDescriptorSet() {}

} // namespace raum::rhi