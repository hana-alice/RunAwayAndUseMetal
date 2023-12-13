#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIQueue {
public:
    explicit RHIQueue(const QueueInfo&, RHIDevice*) {}

protected:
    virtual ~RHIQueue() = 0;
};

inline RHIQueue::~RHIQueue() {}

} // namespace raum::rhi