#pragma once
#include "RHIRenderEncoder.h"

namespace raum::rhi {
class CommandBuffer;
class GraphicsPipeline;
class RenderPass;
class RenderEncoder : public RHIRenderEncoder {
public:
    explicit RenderEncoder(CommandBuffer* commandBuffer);
    RenderEncoder(const RenderEncoder&) = delete;
    RenderEncoder& operator=(const RenderEncoder&) = delete;
    RenderEncoder(RenderEncoder&&) = delete;
    ~RenderEncoder();

    void beginRenderPass(const RenderPassBeginInfo& info) override;
    void nextSubpass() override;
    void endRenderPass() override;
    void bindPipeline(RHIGraphicsPipeline* pipeline) override;
    void setViewport(const Viewport& vp) override;
    void setScissor(const Rect2D& rect) override;
    void setLineWidth(float width) override;
    void setDepthBias(float constantFactor, float clamp, float slopeFactor) override;
    void setBlendConstant(float r, float g, float b, float a) override;
    void setDepthBounds(float min, float max) override;
    void setStencilCompareMask(FaceMode face, uint32_t mask) override;
    void setStencilReference(FaceMode face, uint32_t ref) override;
    void bindDescriptorSet(RHIDescriptorSet* descriptorSet, uint32_t index, uint32_t* dynamicOffsets, uint32_t dynOffsetCount) override;
    void bindIndexBuffer(RHIBuffer* indexBuffer, uint32_t offset, IndexType type) override;
    void bindVertexBuffer(RHIBuffer* vertexBuffer, uint32_t index) override;
    void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
    void drawIndexed(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t vertexOffset, uint32_t firstInstance) override;
    void drawIndirect(RHIBuffer* indirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride) override;
    void drawIndexedIndirect(RHIBuffer* indirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride) override;
    void pushConstants(ShaderStage stage, uint32_t offset, void* data, uint32_t size) override;

    void clearAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, ClearValue* value, ClearRect* rects, uint32_t recNum) override;

private:
    CommandBuffer* _commandBuffer{nullptr};
    GraphicsPipeline* _graphicsPipeline{nullptr};
    RenderPass* _renderPass{nullptr};
};

} // namespace raum::rhi