#include "VKRenderEncoder.h"
#include "VKCommandBuffer.h"
#include "VKFrameBuffer.h"
#include "VKRenderPass.h"
#include "VKComputeEncoder.h"
#include "VKBlitEncoder.h"
#include "VKQueue.h"
#include "VKGraphicsPipeline.h"
#include "VKDevice.h"
#include "VKUtils.h"
#include "VKDescriptorSet.h"
#include "VKPipelineLayout.h"
#include "VKBuffer.h"
namespace raum::rhi {

RenderEncoder::RenderEncoder(CommandBuffer* commandBuffer)
: _commandBuffer(commandBuffer) {
}

RenderEncoder::~RenderEncoder() {
}

void RenderEncoder::beginRenderPass(const RenderPassBeginInfo& info) {
    _renderPass = static_cast<RenderPass*>(info.renderPass);

    VkRenderPassBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass = static_cast<RenderPass*>(info.renderPass)->renderPass();
    beginInfo.framebuffer = static_cast<FrameBuffer*>(info.frameBuffer)->framebuffer();
    beginInfo.renderArea = VkRect2D{
        {info.renderArea.x, info.renderArea.y},
        {info.renderArea.w, info.renderArea.h},
    };

    const auto& attachmentInfos = info.renderPass->attachments();
    std::vector<VkClearValue> clearValues(attachmentInfos.size());
    fillClearColors(clearValues, info.clearColors, attachmentInfos);
    beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    beginInfo.pClearValues = clearValues.data();

    VkSubpassContents contents = _commandBuffer->type() == CommandBufferType::PRIMARY ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;

    vkCmdBeginRenderPass(_commandBuffer->commandBuffer(), &beginInfo, contents);
}

void RenderEncoder::nextSubpass() {
    VkSubpassContents contents = _commandBuffer->type() == CommandBufferType::PRIMARY ? VK_SUBPASS_CONTENTS_INLINE : VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS;
    vkCmdNextSubpass(_commandBuffer->commandBuffer(), contents);
}

void RenderEncoder::endRenderPass() {
    vkCmdEndRenderPass(_commandBuffer->commandBuffer());
}

void RenderEncoder::bindPipeline(RHIGraphicsPipeline* pipeline) {
    _graphicsPipeline = static_cast<GraphicsPipeline*>(pipeline);
    vkCmdBindPipeline(_commandBuffer->commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline->pipeline());
}

void RenderEncoder::setViewport(const Viewport& viewport) {
    VkViewport vp{
        static_cast<float>(viewport.rect.x), 
        static_cast<float>(viewport.rect.y),
        static_cast<float>(viewport.rect.w),
        static_cast<float>(viewport.rect.h),
        viewport.minDepth,
        viewport.maxDepth,
    };
    vkCmdSetViewport(_commandBuffer->commandBuffer(), 0, 1, &vp);
}

void RenderEncoder::setScissor(const Rect2D& scissor) {
    VkRect2D sc{
        scissor.x,
        scissor.y,
        scissor.w,
        scissor.h,
    };
    vkCmdSetScissor(_commandBuffer->commandBuffer(), 0, 1, &sc);
}

void RenderEncoder::setLineWidth(float width) {
    vkCmdSetLineWidth(_commandBuffer->commandBuffer(), width);
}

void RenderEncoder::setDepthBias(float constantFactor, float clamp, float slopeFactor) {
    vkCmdSetDepthBias(_commandBuffer->commandBuffer(), constantFactor, clamp, slopeFactor);
}

void RenderEncoder::setBlendConstant(float r, float g, float b, float a) {
    float bc[4] = {r, g, b, a};
    vkCmdSetBlendConstants(_commandBuffer->commandBuffer(), bc);
}

void RenderEncoder::setDepthBounds(float min, float max) {
    vkCmdSetDepthBounds(_commandBuffer->commandBuffer(), min, max);
}

void RenderEncoder::setStencilCompareMask(FaceMode face, uint32_t compareMask) {
    VkStencilFaceFlags faceFlag = stencilFaceFlags(face);
    vkCmdSetStencilCompareMask(_commandBuffer->commandBuffer(), faceFlag, compareMask);
}

void RenderEncoder::setStencilReference(FaceMode face, uint32_t ref) {
    VkStencilFaceFlags faceFlag = stencilFaceFlags(face);
    vkCmdSetStencilReference(_commandBuffer->commandBuffer(), faceFlag, ref);
}

void RenderEncoder::bindDescriptorSet(RHIDescriptorSet* descriptorSet, uint32_t index, uint32_t* dynamicOffsets, uint32_t dynOffsetCount) {
    auto* descSet = static_cast<DescriptorSet*>(descriptorSet)->descriptorSet();
    VkDescriptorSet kSet = static_cast<DescriptorSet*>(descriptorSet)->descriptorSet();
    vkCmdBindDescriptorSets(_commandBuffer->commandBuffer(),
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _graphicsPipeline->pipelineLayout()->layout(),
                            index,
                            1,
                            &kSet,
                            dynOffsetCount,
                            dynamicOffsets);
}

void RenderEncoder::bindIndexBuffer(RHIBuffer* buffer, uint32_t offset, IndexType type) {
    auto* kBuffer = static_cast<Buffer*>(buffer);
    vkCmdBindIndexBuffer(_commandBuffer->commandBuffer(), kBuffer->buffer(), offset, indexType(type));
}

void RenderEncoder::bindVertexBuffer(RHIBuffer* buffer, uint32_t index) {
    auto* kBuffer = static_cast<Buffer*>(buffer);
    VkBuffer buff = kBuffer->buffer();
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(_commandBuffer->commandBuffer(), 0, 1, &buff, &offset);
}

void RenderEncoder::draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
    vkCmdDraw(_commandBuffer->commandBuffer(), vertexCount, instanceCount, firstVertex, firstInstance);
}

void RenderEncoder::drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t vertexOffset, uint32_t firstInstance) {
    vkCmdDrawIndexed(_commandBuffer->commandBuffer(), indexCount, instanceCount, firstVertex, vertexOffset, firstInstance);
}

void RenderEncoder::drawIndirect(RHIBuffer* buffer, uint32_t offset, uint32_t drawCount, uint32_t stride) {
    auto* kBuffer = static_cast<Buffer*>(buffer);
    vkCmdDrawIndirect(_commandBuffer->commandBuffer(), kBuffer->buffer(), offset, drawCount, stride);
}

void RenderEncoder::drawIndexedIndirect(RHIBuffer* buffer, uint32_t offset, uint32_t drawCount, uint32_t stride) {
    auto* kBuffer = static_cast<Buffer*>(buffer);
    vkCmdDrawIndexedIndirect(_commandBuffer->commandBuffer(), kBuffer->buffer(), offset, drawCount, stride);
}

void RenderEncoder::pushConstants(ShaderStage stage, uint32_t offset, void* data, uint32_t size) {
    VkShaderStageFlags stageFlag = shaderStageFlags(stage);
    vkCmdPushConstants(_commandBuffer->commandBuffer(), _graphicsPipeline->pipelineLayout()->layout(), stageFlag, offset, size, static_cast<uint32_t*>(data));
}

void RenderEncoder::clearAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, ClearValue * clearValues, ClearRect* rects, uint32_t recNum) {
    std::vector<VkClearAttachment> attachments(attachmentNum);
    fillClearAttachment(attachments, clearValues, attachmentIndices, attachmentNum, _renderPass->attachments());
    std::vector<VkClearRect> clearRects(recNum);
    fillClearRect(clearRects, rects, recNum);
    
    vkCmdClearAttachments(_commandBuffer->commandBuffer(), attachmentNum, attachments.data(), recNum, clearRects.data());
}


} // namespace raum::rhi