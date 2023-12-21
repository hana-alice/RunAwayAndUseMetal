#include "VKDescriptorPool.h"
#include "VKDevice.h"
#include "VKUtils.h"
#include "VKDescriptorSet.h"
namespace raum::rhi {

DescriptorPool::DescriptorPool(const DescriptorPoolInfo& info, RHIDevice* device)
: RHIDescriptorPool(info, device), _device(static_cast<Device*>(device)) {
    VkDescriptorPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    createInfo.maxSets = info.maxSets;

    std::vector<VkDescriptorPoolSize> sizes(info.pools.size());
    for (size_t i = 0; i < info.pools.size(); i++) {
        sizes[i].type = descriptorType(info.pools[i].type);
        sizes[i].descriptorCount = info.pools[i].descriptorCount;
    }

    createInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
    createInfo.pPoolSizes = sizes.data();
    vkCreateDescriptorPool(_device->device(), &createInfo, nullptr, &_pool);
}

DescriptorPool::~DescriptorPool() {
    vkDestroyDescriptorPool(_device->device(), _pool, nullptr);
}

RHIDescriptorSet* DescriptorPool::makeDescriptorSet(const DescriptorSetInfo& info) {
    return new DescriptorSet(info, this, _device);
}

}