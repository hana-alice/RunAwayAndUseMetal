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

//struct Semaphores {
//    Semaphores(VkDevice device) : _device(device) {}
//
//    [[nodiscard]] VkSemaphore allocate() {
//        if (_availables.empty()) {
//            VkSemaphore sem;
//            VkSemaphoreCreateInfo info{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
//            vkCreateSemaphore(_device, &info, nullptr, &sem);
//            _inUse.emplace_back(sem);
//            return sem;
//        } else {
//            auto sem = _availables.front();
//            _availables.pop();
//            _inUse.emplace_back(sem);
//            return sem;
//        }
//    }
//
//    void reset() {
//        for(auto sem : _inUse) {
//            _availables.emplace(sem);
//        }
//        _inUse.clear();
//    }
//
//    const std::vector<VkSemaphore>& inUse() const {
//        return _inUse;
//    }
//
//    ~Semaphores() {
//        vkDeviceWaitIdle(_device);
//        for(auto sem : _inUse) {
//            vkDestroySemaphore(_device, _inUse.front(), nullptr);
//        }
//        while(!_availables.empty()) {
//            vkDestroySemaphore(_device, _availables.front(), nullptr);
//            _availables.pop();
//        }
//    }
//
//    VkDevice _device;
//    std::vector<VkSemaphore> _inUse;
//    std::queue<VkSemaphore> _availables;
//};

class Queue : public RHIQueue {
public:
    uint32_t index() const override { return _index; }

    void submit(bool signal) override;

    void enqueue(RHICommandBuffer* commandBuffer) override;

    void bindSparse(const SparseBindingInfo& info, SparseType type) override;

    void addCompleteHandler(std::function<void()>&& func);

    VkQueue queue() const { return _vkQueue; }

    void addWait(RHISemaphore* sem) override;

    RHISemaphore* getSignal() override;

    ~Queue();

private:
    Queue(const QueueInfo& info, Device* device);
    void initQueue();

    VkQueue _vkQueue{VK_NULL_HANDLE};

    QueueInfo _info;
    uint32_t _index{0};
    uint32_t _currFrameIndex{0};
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