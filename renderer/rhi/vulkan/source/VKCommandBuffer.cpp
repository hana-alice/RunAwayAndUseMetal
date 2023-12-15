#include "VkCommandBuffer.h"
#include "VKQueue.h"
#include "VKDevice.h"
#include "VKUtils.h"
#include <type_traits>
#include "VKRenderEncoder.h"
#include "VKComputeEncoder.h"
#include "VKBlitEncoder.h"
namespace raum::rhi {
CommandBuffer::CommandBuffer(const CommandBufferInfo& info, RHIDevice* device)
: RHICommandBuffer(info, device), _device(static_cast<Device*>(device)), _info(info) {
    auto* queue = static_cast<Queue*>(info.queue);
    auto commandPool = queue->commandPool();
    VkCommandBufferAllocateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    createInfo.commandBufferCount = 1;
    createInfo.commandPool = commandPool;
    createInfo.level = commandBufferLevel(info.type);
    vkAllocateCommandBuffers(_device->device(), &createInfo, &_commandBuffer);
}

CommandBuffer::~CommandBuffer() {
    auto commandPool = static_cast<Queue*>(_info.queue)->commandPool();
    vkFreeCommandBuffers(_device->device(), commandPool, 1, &_commandBuffer);
}

RHIRenderEncoder* CommandBuffer::makeRenderEncoder() {
    return new RenderEncoder(this);
}

RHIComputeEncoder* CommandBuffer::makeComputeEncoder() {
    return nullptr;
}

RHIBlitEncoder* CommandBuffer::makeBlitEncoder() {
    return nullptr;
}

void CommandBuffer::enqueue() {
    static_cast<Queue*>(_info.queue)->enqueue(this);
    _enqueued = true;
}

void CommandBuffer::commit() {
    if (!_enqueued) {
        static_cast<Queue*>(_info.queue)->enqueue(this);
    }
    _status = CommandBufferStatus::COMITTED;
}

void CommandBuffer::reset() {
    vkResetCommandBuffer(_commandBuffer, VkCommandBufferResetFlagBits{});
}

} // namespace raum::rhi