#pragma once
#include "RHIQueue.h"
#include "VKDefine.h"
#include "VKBuffer.h"
namespace raum::rhi {

class Device;
class Swapchain;
class CommandBuffer;
class Queue : public RHIQueue {
public:
    uint32_t index() const override { return _index; }

    void submit() override;

    void enqueue(RHICommandBuffer* commandBuffer) override;

    void addCompleteHandler(const std::function<void()>& func);

    VkSemaphore popCommandSemaphore();

    ~Queue();
private:
    Queue(const QueueInfo& info, Device* device);

    void initPresentQueue(uint32_t presentCount);
    void initCommandQueue();

    void setPresentSemaphore(VkSemaphore semaphore);

    VkQueue _vkQueue{VK_NULL_HANDLE};

    QueueInfo _info;
    uint32_t _index{0};
    uint32_t _currFrameIndex{0};
    Device* _device{nullptr};

    std::vector<CommandBuffer*> _commandBuffers;
    std::vector<VkSemaphore> _commandSemaphores;
    std::vector<VkSemaphore> _presentSemaphores;
    std::vector<VkFence> _frameFence;

    VkSemaphore _currCommandSemaphore;

    std::array<std::vector<std::function<void()>>, FRAMES_IN_FLIGHT> _completeHandlers;

    StagingBuffer* _stagingBuffer{nullptr};

    friend class Device;
    friend class Swapchain;
};

} // namespace raum::rhi