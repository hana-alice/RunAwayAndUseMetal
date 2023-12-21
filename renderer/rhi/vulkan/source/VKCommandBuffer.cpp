#include "VkCommandBuffer.h"
#include <type_traits>
#include "VKBlitEncoder.h"
#include "VKBuffer.h"
#include "VKComputeEncoder.h"
#include "VKDevice.h"
#include "VKImage.h"
#include "VKQueue.h"
#include "VKRenderEncoder.h"
#include "VKUtils.h"
#include "VKDescriptorSet.h"
#include "VKCommandPool.h"
namespace raum::rhi {
CommandBuffer::CommandBuffer(const CommandBufferInfo& info, CommandPool* commandPool, RHIDevice* device)
: RHICommandBuffer(info, device),
  _device(static_cast<Device*>(device)),
  _commandPool(commandPool) {
    VkCommandBufferAllocateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    createInfo.commandBufferCount = 1;
    createInfo.commandPool = commandPool->commandPool();
    createInfo.level = commandBufferLevel(info.type);
    vkAllocateCommandBuffers(_device->device(), &createInfo, &_commandBuffer);
}

CommandBuffer::~CommandBuffer() {
    vkFreeCommandBuffers(_device->device(), _commandPool->commandPool(), 1, &_commandBuffer);
}

RHIRenderEncoder* CommandBuffer::makeRenderEncoder() {
    return new RenderEncoder(this);
}

RHIComputeEncoder* CommandBuffer::makeComputeEncoder() {
    return nullptr;
}

RHIBlitEncoder* CommandBuffer::makeBlitEncoder() {
    return new BlitEncoder(this);
}

void CommandBuffer::begin(const CommandBufferBeginInfo& info) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = commandBufferUsage(info.flags);
    vkBeginCommandBuffer(_commandBuffer, &beginInfo);
}

void CommandBuffer::enqueue(RHIQueue* queue) {
    queue->enqueue(this);
    _enqueued = true;
}

void CommandBuffer::commit(RHIQueue* queue) {
    if (!_enqueued) {
        queue->enqueue(this);
    }
    _status = CommandBufferStatus::COMITTED;
    vkEndCommandBuffer(_commandBuffer);
}

void CommandBuffer::reset() {
    vkResetCommandBuffer(_commandBuffer, VkCommandBufferResetFlagBits{});
}

void CommandBuffer::appendImageBarrier(const ImageBarrierInfo& info) {
    _imageBarriers.emplace_back(info);
}

void CommandBuffer::appendBufferBarrier(const BufferBarrierInfo& info) {
    _bufferBarriers.emplace_back(info);
}

void CommandBuffer::applyBarrier(DependencyFlags flags) {
    std::vector<VkBufferMemoryBarrier> bufferBarriers(_bufferBarriers.size());
    std::vector<VkImageMemoryBarrier> imageBarriers(_imageBarriers.size());
    VkPipelineStageFlags srcStageMask{VK_PIPELINE_STAGE_NONE};
    VkPipelineStageFlags dstStageMask{VK_PIPELINE_STAGE_NONE};
    for (size_t i = 0; i < _bufferBarriers.size(); ++i) {
        srcStageMask |= pipelineStageFlags(_bufferBarriers[i].srcStage);
        dstStageMask |= pipelineStageFlags(_bufferBarriers[i].dstStage);
        bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufferBarriers[i].buffer = static_cast<Buffer*>(_bufferBarriers[i].buffer)->buffer();
        bufferBarriers[i].srcAccessMask = accessFlags(_bufferBarriers[i].srcAccessFlag);
        bufferBarriers[i].dstAccessMask = accessFlags(_bufferBarriers[i].dstAccessFlag);
        bufferBarriers[i].srcQueueFamilyIndex = _bufferBarriers[i].srcQueueIndex;
        bufferBarriers[i].dstQueueFamilyIndex = _bufferBarriers[i].dstQueueIndex;
        bufferBarriers[i].offset = _bufferBarriers[i].offset;
        bufferBarriers[i].size = _bufferBarriers[i].size;
    }

    for (size_t i = 0; i < _imageBarriers.size(); ++i) {
        srcStageMask |= pipelineStageFlags(_imageBarriers[i].srcStage);
        dstStageMask |= pipelineStageFlags(_imageBarriers[i].dstStage);
        imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageBarriers[i].image = static_cast<Image*>(_imageBarriers[i].image)->image();
        imageBarriers[i].srcAccessMask = accessFlags(_imageBarriers[i].srcAccessFlag);
        imageBarriers[i].dstAccessMask = accessFlags(_imageBarriers[i].dstAccessFlag);
        imageBarriers[i].oldLayout = imageLayout(_imageBarriers[i].oldLayout);
        imageBarriers[i].newLayout = imageLayout(_imageBarriers[i].newLayout);
        imageBarriers[i].srcQueueFamilyIndex = _imageBarriers[i].srcQueueIndex;
        imageBarriers[i].dstQueueFamilyIndex = _imageBarriers[i].dstQueueIndex;
        imageBarriers[i].subresourceRange.aspectMask = aspectMask(_imageBarriers[i].range.aspect);
        imageBarriers[i].subresourceRange.baseArrayLayer = _imageBarriers[i].range.firstSlice;
        imageBarriers[i].subresourceRange.layerCount = _imageBarriers[i].range.sliceCount;
        imageBarriers[i].subresourceRange.baseMipLevel = _imageBarriers[i].range.firstMip;
        imageBarriers[i].subresourceRange.levelCount = _imageBarriers[i].range.mipCount;
    }

    vkCmdPipelineBarrier(_commandBuffer, srcStageMask, dstStageMask, dependencyFlags(flags),
                         0, nullptr,
                         static_cast<uint32_t>(bufferBarriers.size()), bufferBarriers.data(),
                         static_cast<uint32_t>(imageBarriers.size()), imageBarriers.data());
}

} // namespace raum::rhi