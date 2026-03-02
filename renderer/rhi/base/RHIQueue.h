#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
#include <span>

namespace raum::rhi {
class RHIDevice;
class RHICommandBuffer;
class RHISemaphore;

class RHIQueue: public RHIResource  {
public:
    virtual void submit(bool enableFrameFence) = 0;
    virtual void enqueue(RHICommandBuffer*) = 0;
    virtual uint32_t index() const = 0;
    virtual void addWait(RHISemaphore* sem) = 0;
    virtual void addSignal(RHISemaphore* sem) = 0;
    virtual void bindSparse(const SparseBindingInfo& info, SparseType type) = 0;

    virtual void* queue() { return nullptr; }
protected:
    virtual ~RHIQueue() = 0;
};

inline RHIQueue::~RHIQueue() {}

} // namespace raum::rhi