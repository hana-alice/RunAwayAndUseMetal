#pragma once
#include "RHIQueue.h"
#include "VKDefine.h"
namespace raum::rhi {

class Device;
class Swapchain; // vs required: friend class must declared before first seen
class CommandBuffer;
class Queue : public RHIQueue {
public:
    uint32_t index() { return _index; }

    VkCommandPool commandPool() { return _commandPool; }

    RHICommandBuffer* makeCommandBuffer() override;

    //void executeCommandBuffers(RHICommandBuffer* cmdBUffers, uint32_t count) override;

    void submit() override;

    void enqueue(RHICommandBuffer* commandBuffer) override;

    ~Queue();
private:
    Queue(const QueueInfo& info, Device* device);

    void initPresentQueue(uint32_t presentCount);

    VkQueue _vkQueue;
    VkCommandPool _commandPool;

    QueueInfo _info;
    uint32_t _index{0};
    uint32_t _currFrameIndex{0};
    Device* _device{nullptr};

    std::vector<CommandBuffer*> _commandBuffers;
    std::vector<VkSemaphore> _commandBufferSemaphores;
    std::vector<VkSemaphore> _presentSemaphores;
    std::vector<VkFence> _frameFence;

    friend class Device;
    friend class Swapchain;
};

} // namespace raum::rhi