#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIQueue {
public:
    explicit RHIQueue(const QueueInfo&) {}
    virtual ~RHIQueue() = 0;
};
} // namespace raum::rhi