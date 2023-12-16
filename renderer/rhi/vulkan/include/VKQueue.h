#pragma once
#include "RHIQueue.h"
#include "VKDefine.h"
namespace raum::rhi {

class Device;
class Swapchain;
class CommandBuffer;
class Queue : public RHIQueue {
public:
    uint32_t index() { return _index; }

    VkCommandPool commandPool() { return _commandPool; }

    RHICommandBuffer* makeCommandBuffer() override;

    void submit() override;

    void enqueue(RHICommandBuffer* commandBuffer) override;

    VkSemaphore presentSemaphore();
    VkSemaphore renderSemaphore();

    ~Queue();
private:
    Queue(const QueueInfo& info, Device* device);

    void initPresentQueue(uint32_t presentCount);
    void initCommandQueue();

    VkQueue _vkQueue;
    VkCommandPool _commandPool;

    QueueInfo _info;
    uint32_t _index{0};
    uint32_t _currFrameIndex{0};
    Device* _device{nullptr};

    std::vector<CommandBuffer*> _commandBuffers;
    std::vector<VkSemaphore> _renderingSemaphores;
    std::vector<VkSemaphore> _presentSemaphores;
    std::vector<VkFence> _frameFence;

    friend class Device;
    friend class Swapchain;
};

} // namespace raum::rhi