#pragma once
#include "RHIQueue.h"
#include "VKDefine.h"
namespace raum::rhi {

class Device;
class Swapchain;
class CommandBuffer;
class Queue : public RHIQueue {
public:
    uint32_t index() const override { return _index; }

    void submit() override;

    void enqueue(RHICommandBuffer* commandBuffer) override;

    VkSemaphore presentSemaphore();
    VkSemaphore popCommandSemaphore();

    ~Queue();
private:
    Queue(const QueueInfo& info, Device* device);

    void initPresentQueue(uint32_t presentCount);
    void initCommandQueue();

    VkQueue _vkQueue;

    QueueInfo _info;
    uint32_t _index{0};
    uint32_t _currFrameIndex{0};
    Device* _device{nullptr};

    std::vector<CommandBuffer*> _commandBuffers;
    std::vector<VkSemaphore> _commandSemaphores;
    std::vector<VkSemaphore> _presentSemaphores;
    std::vector<VkFence> _frameFence;

    VkSemaphore _currCommandSemaphore;

    friend class Device;
    friend class Swapchain;
};

} // namespace raum::rhi