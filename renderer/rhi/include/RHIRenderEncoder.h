#pragma once
#include "RHIDefine.h"
namespace raum::rhi {

class RHIDevice;
class RHIGraphicsPipeline;
class RHIDescriptorSet;
class RHIBuffer;
class RHIPipelineLayout;
class RHIRenderEncoder {
public:
    virtual ~RHIRenderEncoder() = 0;
    virtual void beginRenderPass() = 0;
    virtual void nextSubpass() = 0;
    virtual void endRenderPass() = 0;
    virtual void bindPipeline(RHIGraphicsPipeline* pipeline) = 0;
    virtual void setViewport(const Viewport& vp) = 0;
    virtual void setScissor(const Rect2D& rect) = 0;
    virtual void setLineWidth(float width) = 0;
    virtual void setDepthBias(float constantFactor, float clamp, float slopeFactor) = 0;
    virtual void setBlendConstant(float r, float g, float b, float a) = 0;
    virtual void setDepthBounds(float min, float max) = 0;
    virtual void setStencilCompareMask(FaceMode face, uint32_t mask) = 0;
    virtual void setStencilReference(FaceMode face, uint32_t ref) = 0;
    virtual void bindDescriptorSet(RHIDescriptorSet* descriptorSet, uint32_t index, uint32_t* dynamicOffsets, uint32_t dynOffsetCount) = 0;
    virtual void bindIndexBuffer(RHIBuffer* indexBuffer, uint32_t offset, IndexType type) = 0;
    virtual void bindVertexBuffer(RHIBuffer* vertexBuffer, uint32_t index) = 0;
    virtual void draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
    virtual void drawIndexed(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t vertexOffset, uint32_t firstInstance) = 0;
    virtual void drawIndirect(RHIBuffer* indirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride) = 0;
    virtual void drawIndexedIndirect(RHIBuffer* indirectBuffer, uint32_t offset, uint32_t drawCount, uint32_t stride) = 0;
    virtual void pushConstants(RHIPipelineLayout* pipelineLayout, ShaderStage stage, void* data, uint32_t size) = 0;

    virtual void clearColorAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, uint32_t value, ClearRect* rects, uint32_t recNum) = 0;
    virtual void clearColorAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, int32_t value, ClearRect* rects, uint32_t recNum) = 0;
    virtual void clearColorAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, float value, ClearRect* rects, uint32_t recNum) = 0;

    virtual void clearDepthAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, float value, ClearRect* rects, uint32_t recNum) = 0;
    virtual void clearDepthAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, uint32_t value, ClearRect* rects, uint32_t recNum) = 0;

    virtual void clearStencilAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, uint32_t value, ClearRect* rects, uint32_t recNum) = 0;
    virtual void clearStencilAttachment(uint32_t* attachmentIndices, uint32_t attachmentNum, int32_t value, ClearRect* rects, uint32_t recNum) = 0;
};

inline RHIRenderEncoder::~RHIRenderEncoder() {}

} // namespace raum::rhi