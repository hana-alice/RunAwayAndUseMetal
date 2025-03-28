#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"

namespace raum::rhi {
class RHIQueue;
class RHIRenderEncoder;
class RHIBlitEncoder;
class RHIComputeEncoder;
class RHIDevice;
class RHICommandBuffer: public RHIResource  {
public:
    virtual ~RHICommandBuffer() = 0;

    virtual RHIRenderEncoder* makeRenderEncoder(RenderEncoderHint hint) = 0;
    virtual RHIRenderEncoder* makeRenderEncoder() = 0;
    virtual RHIBlitEncoder* makeBlitEncoder() = 0;
    virtual RHIComputeEncoder* makeComputeEncoder() = 0;

    virtual void begin(const CommandBufferBeginInfo&) = 0;

    // add to queue to hold place in order
    virtual void enqueue(RHIQueue*) = 0;

    // commit to queue, enqueue automatically if not enqueued
    virtual void commit() = 0;

    // reuse
    virtual void reset() = 0;

    virtual void appendImageBarrier(const ImageBarrierInfo&) = 0;

    virtual void appendBufferBarrier(const BufferBarrierInfo&) = 0;

    virtual void appendExecutionBarrier(const ExecutionBarrier&) = 0;

    virtual void applyBarrier(DependencyFlags flags) = 0;

    virtual void onComplete(std::function<void()>&&) = 0;

protected:
    explicit RHICommandBuffer(const CommandBufferInfo& info, RHIDevice* device) {}
};

inline RHICommandBuffer::~RHICommandBuffer() {}

} // namespace raum::rhi