#pragma once
#include <queue>
#include "RHIQueue.h"
#include "VKBuffer.h"
#include "VKDefine.h"
namespace raum::rhi {

class Device;
class Swapchain;
class CommandBuffer;
class Semaphore;

class Queue : public RHIQueue {
public:
    uint32_t index() const override { return _index; }

    void submit(bool enableFrameFence) override;

    void enqueue(RHICommandBuffer* commandBuffer) override;

    void bindSparse(const SparseBindingInfo& info, SparseType type) override;

    void addCompleteHandler(std::function<void()>&& func);

    VkQueue queue() const { return _vkQueue; }

    void addWait(RHISemaphore* sem) override;

    void addSignal(RHISemaphore* sem) override;

    uint16_t frameIndex() const { return _currFrameIndex; }

    void increaseFrameIndex();

    ~Queue();

private:
    Queue(const QueueInfo& info, Device* device);
    void initQueue();

    VkQueue _vkQueue{VK_NULL_HANDLE};

    QueueInfo _info;
    uint32_t _index{0};
    uint64_t _currFrameIndex{0};
    Device* _device{nullptr};

    std::vector<CommandBuffer*> _commandBuffers;
    std::vector<VkFence> _frameFence;

    std::vector<Semaphore*> _waits;
    std::vector<Semaphore*> _signals;

    std::array<std::vector<std::function<void()>>, FRAMES_IN_FLIGHT> _completeHandlers;

//    StagingBuffer* _stagingBuffer{nullptr};

    friend class Device;
    friend class Swapchain;
};

} // namespace raum::rhi