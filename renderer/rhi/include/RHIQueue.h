#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHICommandBuffer;
class RHIQueue {
public:
    explicit RHIQueue(const QueueInfo&, RHIDevice*) {}

    virtual RHICommandBuffer* makeCommandBuffer(const CommandBufferInfo& info) = 0;
    virtual void submit() = 0;
    virtual void enqueue(RHICommandBuffer*) = 0;
    virtual uint32_t index() const = 0;

    virtual ~RHIQueue() = 0;

protected:
};

inline RHIQueue::~RHIQueue() {}

} // namespace raum::rhi