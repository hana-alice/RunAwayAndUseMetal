#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHICommandBuffer;
class RHIQueue {
public:
    explicit RHIQueue(const QueueInfo&, RHIDevice*) {}

    virtual RHICommandBuffer* makeCommandBuffer() = 0;

protected:
    virtual ~RHIQueue() = 0;
};

inline RHIQueue::~RHIQueue() {}

} // namespace raum::rhi