#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIQueue {
public:
    explicit RHIQueue(const QueueInfo&, RHIDevice*) {}
    virtual ~RHIQueue() = 0;
};
} // namespace raum::rhi