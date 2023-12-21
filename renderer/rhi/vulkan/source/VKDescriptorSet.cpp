#include "VKDescriptorSet.h"
#include "VKDevice.h"
#include "VKDescriptorPool.h"
#include "VKDescriptorSetLayout.h"
namespace raum::rhi {
DescriptorSet::DescriptorSet(const DescriptorSetInfo& info, DescriptorPool* pool, RHIDevice* device)
: RHIDescriptorSet(info, device),
  _device(static_cast<Device*>(device)),
  _descriptorPool(pool) {
    
    auto* descriptorSetLayout = static_cast<DescriptorSetLayout*>(info.layout);
    auto kLayout = descriptorSetLayout->layout();

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool->descriptorPool();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &kLayout;

    vkAllocateDescriptorSets(_device->device(), &allocInfo, &_descriptorSet);
}

DescriptorSet::~DescriptorSet() {
    vkFreeDescriptorSets(_device->device(), _descriptorPool->descriptorPool(), 1, &_descriptorSet);
}

}