#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHICommandBuffer;
class RHIQueue {
public:
    virtual void submit() = 0;
    virtual void enqueue(RHICommandBuffer*) = 0;
    virtual uint32_t index() const = 0;

protected:
    virtual ~RHIQueue() = 0;
};

inline RHIQueue::~RHIQueue() {}

} // namespace raum::rhi