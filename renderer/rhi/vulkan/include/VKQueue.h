#pragma once
#include "RHIQueue.h"
#include "VKDefine.h"
namespace raum::rhi {

class Device;
class Queue : public RHIQueue {
public:
    uint32_t index() { return _index; }

private:
    Queue(const QueueInfo& info, Device* device);

    QueueInfo _info;
    uint32_t _index{0};

    VkQueue _vkQueue;

    friend class Device;
};

} // namespace raum::rhi