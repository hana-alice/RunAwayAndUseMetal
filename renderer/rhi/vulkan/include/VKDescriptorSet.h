#pragma once
#include "RHIDescriptorSet.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class DescriptorPool;
class DescriptorSet : public RHIDescriptorSet {
public:
    ~DescriptorSet() override;

    void update(const std::vector<BindingInfo>& info) override;
    void updateBuffer(const BufferBinding& info) override;
    void updateImage(const ImageBinding& info) override;
    void updateSampler(const SamplerBinding& info) override;
    void updateTexelBuffer(const TexelBufferBinding& info) override;

    VkDescriptorSet descriptorSet() const { return _descriptorSet; }

private:
    DescriptorSet(const DescriptorSetInfo& info, DescriptorPool* pool, RHIDevice* device);

    VkDescriptorSet _descriptorSet;
    Device* _device;
    DescriptorPool* _descriptorPool;

    friend class DescriptorPool;
};
} // namespace raum::rhi