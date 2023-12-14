#pragma once
#include "RHIQueue.h"
#include "VKDefine.h"
namespace raum::rhi {

class Device;
class Queue : public RHIQueue {
public:
    uint32_t index() { return _index; }

    VkCommandPool commandPool() { return _commandPool; }

    RHICommandBuffer* makeCommandBuffer() override;

    ~Queue();

private:
    Queue(const QueueInfo& info, Device* device);

    QueueInfo _info;
    uint32_t _index{0};
    Device* _device{nullptr};

    VkQueue _vkQueue;
    VkCommandPool _commandPool;

    friend class Device;
};

} // namespace raum::rhi