#pragma once
#include "RHIDefine.h"

namespace raum::rhi {
class RHIDevice;
class RHIRenderEncoder;
class RHIBlitEncoder;
class RHIComputeEncoder;
class RHICommandBuffer {
public:
    virtual ~RHICommandBuffer() = 0;

    virtual RHIRenderEncoder* makeRenderEncoder() = 0;
    virtual RHIBlitEncoder* makeBlitEncoder() = 0;
    virtual RHIComputeEncoder* makeComputeEncoder() = 0;

    virtual void executeCommandBuffers(RHICommandBuffer* cmdBUffers, uint32_t count) = 0;

    // add to queue to hold place in order
    virtual void enqueue() = 0;

    // commit to queue, enqueue automatically if not enqueued
    virtual void commit() = 0;

protected:
    explicit RHICommandBuffer(const CommandBufferInfo& info) {}
};

inline RHICommandBuffer::~RHICommandBuffer() {}

} // namespace raum::rhi