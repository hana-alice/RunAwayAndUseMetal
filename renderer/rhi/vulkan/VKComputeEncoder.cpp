#include "VKComputeEncoder.h"
#include "VKBuffer.h"
#include "VKCommandBuffer.h"
#include "VKComputePipeline.h"
#include "VKDescriptorSet.h"
#include "VKPipelineLayout.h"
namespace raum::rhi {

ComputeEncoder::ComputeEncoder(CommandBuffer* commandBuffer)
: _commandBuffer(commandBuffer) {}

ComputeEncoder::~ComputeEncoder() {
}

void ComputeEncoder::bindPipeline(RHIComputePipeline* pipeline) {
    _pipeline = static_cast<ComputePipeline*>(pipeline);
    vkCmdBindPipeline(_commandBuffer->commandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline->pipeline());
}

void ComputeEncoder::bindDescriptorSet(RHIDescriptorSet* descriptorSet, uint32_t index, uint32_t* dynamicOffsets, uint32_t dynOffsetCount) {
    VkDescriptorSet kSet = static_cast<DescriptorSet*>(descriptorSet)->descriptorSet();
    vkCmdBindDescriptorSets(_commandBuffer->commandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, _pipeline->pipelineLayout()->layout(), index, 1, &kSet, dynOffsetCount, dynamicOffsets);
}

void ComputeEncoder::dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) {
    vkCmdDispatch(_commandBuffer->commandBuffer(), groupCountX, groupCountY, groupCountZ);
}

void ComputeEncoder::dispatchIndirect(RHIBuffer* indirectBuffer, uint32_t offset) {
    auto idBuffer = static_cast<Buffer*>(indirectBuffer)->buffer();
    vkCmdDispatchIndirect(_commandBuffer->commandBuffer(), idBuffer, offset);
}

void ComputeEncoder::pushConstants(ShaderStage stage, uint32_t offset, void* data, uint32_t size) {
    vkCmdPushConstants(_commandBuffer->commandBuffer(), _pipeline->pipelineLayout()->layout(), static_cast<VkShaderStageFlags>(stage), offset, size, data);
}

} // namespace raum::rhi