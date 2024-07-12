#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHICommandBuffer;
class RHIQueue: public RHIResource  {
public:
    virtual void submit(bool signal) = 0;
    virtual void enqueue(RHICommandBuffer*) = 0;
    virtual uint32_t index() const = 0;
    virtual void addWait(RHISemaphore* sem) = 0;
    virtual RHISemaphore* getSignal() = 0;

protected:
    virtual ~RHIQueue() = 0;
};

inline RHIQueue::~RHIQueue() {}

} // namespace raum::rhi