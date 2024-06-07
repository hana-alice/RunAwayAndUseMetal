#include "VKCommandPool.h"
#include "VKDevice.h"
#include "VKCommandBuffer.h"
namespace raum::rhi {

CommandPool::CommandPool(const CommandPoolInfo& info, RHIDevice* device)
: RHICommandPool(info, device), _device(static_cast<Device*>(device)) {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = info.queueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vkCreateCommandPool(_device->device(), &poolInfo, nullptr, &_pool);
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(_device->device(), _pool, nullptr);
}

RHICommandBuffer* CommandPool::makeCommandBuffer(const CommandBufferInfo& info) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandBufferCount = 1;
    allocInfo.level = info.type == CommandBufferType::PRIMARY ? VK_COMMAND_BUFFER_LEVEL_PRIMARY
                                                              : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandPool = _pool;
    
    return new CommandBuffer(info, this, _device);
}

}