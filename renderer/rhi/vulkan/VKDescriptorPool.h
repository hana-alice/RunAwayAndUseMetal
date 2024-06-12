#pragma once
#include "RHIDescriptorPool.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class DescriptorPool : public RHIDescriptorPool {
public:
    explicit DescriptorPool(const DescriptorPoolInfo& info, RHIDevice* device);
    ~DescriptorPool() override;

    RHIDescriptorSet* makeDescriptorSet(const DescriptorSetInfo&) override;

    VkDescriptorPool descriptorPool() const { return _pool; }
private:
    VkDescriptorPool _pool;
    Device* _device;
};

}