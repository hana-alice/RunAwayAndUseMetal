#pragma once
#include "RHIQueue.h"
#include "VKDefine.h"
namespace raum::rhi {

class Device;
class CommandBuffer;
class Queue : public RHIQueue {
public:
    uint32_t index() { return _index; }

    VkCommandPool commandPool() { return _commandPool; }

    RHICommandBuffer* makeCommandBuffer() override;

    void submit() override;

    void enqueue(CommandBuffer* commandBuffer);

    ~Queue();

private:
    Queue(const QueueInfo& info, Device* device);

    VkQueue _vkQueue;
    VkCommandPool _commandPool;

    QueueInfo _info;
    uint32_t _index{0};
    Device* _device{nullptr};
    std::vector<CommandBuffer*> _cmdBufferQueue;

    friend class Device;
};

} // namespace raum::rhi