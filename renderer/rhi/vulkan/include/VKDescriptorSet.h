#pragma once
#include "RHIDescriptorSet.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class DescriptorPool;
class DescriptorSet : public RHIDescriptorSet {
public:
    ~DescriptorSet() override;

	VkDescriptorSet descriptorSet() const { return _descriptorSet; }

private:
    DescriptorSet(const DescriptorSetInfo& info, DescriptorPool* pool, RHIDevice* device);

    VkDescriptorSet _descriptorSet;
    Device* _device;
    DescriptorPool* _descriptorPool;

    friend class DescriptorPool;
};
}