#include "VKQueue.h"
#include <optional>
#include <vector>
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKSemaphore.h"
#include "VKSparseImage.h"
#include "VKUtils.h"
namespace raum::rhi {
Queue::Queue(const QueueInfo& info, Device* device)
: _device(static_cast<Device*>(device)) {
    _info = info;

    uint32_t queueFamilyCount{0};
    auto physicDevice = device->physicalDevice();
    vkGetPhysicalDeviceQueueFamilyProperties(physicDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicDevice, &queueFamilyCount, queueFamilies.data());

    std::optional<uint32_t> index;
    for (size_t i = 0; i < queueFamilies.size(); ++i) {
        const auto& queueFamily = queueFamilies[i];
        if (_info.type == QueueType::GRAPHICS && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            index = static_cast<uint32_t>(i);
            break;
        } else if (_info.type == QueueType::COMPUTE && queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            index = static_cast<uint32_t>(i);
            break;
        } else if (_info.type == QueueType::TRANSFER && queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            index = static_cast<uint32_t>(i);
            break;
        } else if (_info.type == QueueType::SPARSE && queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) {
            index = static_cast<uint32_t>(i);
            break;
        }
    }

    // vs warning
    VK_ENSURE(index.has_value(), "Requested Vulkan queue type is not supported");
    _index = index.value();
}

void Queue::initQueue() {
    _frameFence.resize(FRAMES_IN_FLIGHT);
    for (auto& fence : _frameFence) {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VK_EXPECT(vkCreateFence(_device->device(), &info, nullptr, &fence));
    }
}

Queue::~Queue() {
    if (_vkQueue != VK_NULL_HANDLE) {
        vkQueueWaitIdle(_vkQueue);
        for (auto fence : _frameFence) {
            vkDestroyFence(_device->device(), fence, nullptr);
        }
    }
}

void Queue::enqueue(RHICommandBuffer* cmdBuffer) {
    _commandBuffers.emplace_back(static_cast<CommandBuffer*>(cmdBuffer));
}

void Queue::submit(bool enableFrameFence) {
    VkSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::vector<VkCommandBuffer> cmdBuffers(_commandBuffers.size());
    for (size_t i = 0; i < _commandBuffers.size(); ++i) {
        cmdBuffers[i] = _commandBuffers[i]->commandBuffer();
    }
    info.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
    info.pCommandBuffers = cmdBuffers.data();

    // image available & pre task
    std::vector<VkSemaphore> waitSems;
    std::vector<VkPipelineStageFlags> waitStages;

    if (!_waits.empty()) {
        for (auto* s : _waits) {
            waitSems.emplace_back(s->semaphore());
            waitStages.emplace_back(pipelineStageFlags(s->getStage()));
        }
        info.pWaitSemaphores = waitSems.data();
        info.pWaitDstStageMask = waitStages.data();

        _waits.clear();
    } else {
        info.pWaitSemaphores = nullptr;
        info.pWaitDstStageMask = nullptr;
    }
    info.waitSemaphoreCount = waitSems.size();
    info.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
    std::vector<VkSemaphore> signalSems;
    if (!_signals.empty()) {
        for (auto* s : _signals) {
            signalSems.emplace_back(s->semaphore());
        }
        info.signalSemaphoreCount = _signals.size();
        info.pSignalSemaphores = signalSems.data();
        _signals.clear();
    } else {
        info.signalSemaphoreCount = 0;
        info.pSignalSemaphores = nullptr;
    }

    VkFence lastFence = VK_NULL_HANDLE;
    uint32_t fenceCount = 0;
    if (enableFrameFence) [[likely]] {
        lastFence = _frameFence[(_currFrameIndex + FRAMES_IN_FLIGHT - 1 ) % FRAMES_IN_FLIGHT];
        fenceCount = 1;
    }
    VK_EXPECT(vkQueueSubmit(_vkQueue, 1, &info, lastFence));
    if (fenceCount) {
        VK_EXPECT(vkWaitForFences(_device->device(), fenceCount, &lastFence, VK_TRUE, UINT64_MAX));
        VK_EXPECT(vkResetFences(_device->device(), fenceCount, &lastFence));
        _device->resetStagingBuffer(_index);
    }

    _commandBuffers.clear();
}

void Queue::increaseFrameIndex() {
    _currFrameIndex = (_currFrameIndex + 1) % FRAMES_IN_FLIGHT;
    for (auto& completeFunc : _completeHandlers[_currFrameIndex]) {
        completeFunc();
    }
    _completeHandlers[_currFrameIndex].clear();
}

void Queue::bindSparse(const SparseBindingInfo& info, SparseType type) {
    VkBindSparseInfo bindInfo{.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO};
    bindInfo.bufferBindCount = 0;
    bindInfo.pBufferBinds = nullptr;

    std::vector<VkSparseImageMemoryBindInfo> imageMemBindInfos;
    std::vector<VkSparseImageOpaqueMemoryBindInfo> opaqueBindInfos;
    for (auto img : info.images) {
        auto sparseImage = static_cast<SparseImage*>(img);
        const auto& mipTail = sparseImage->opaqueBind();

        ++bindInfo.imageOpaqueBindCount;
        VkSparseImageOpaqueMemoryBindInfo opaqueBind{};
        opaqueBind.image = sparseImage->image();
        opaqueBind.bindCount = 1;
        opaqueBind.pBinds = &mipTail;
        opaqueBindInfos.emplace_back(opaqueBind);

        if (test(type, SparseType::IMAGE)) {
            const auto& imageBinds = sparseImage->sparseImageMemoryBinds();
            VkSparseImageMemoryBindInfo imageMemoryBindInfo{};
            imageMemoryBindInfo.bindCount = imageBinds.size();
            imageMemoryBindInfo.image = sparseImage->image();
            imageMemoryBindInfo.pBinds = imageBinds.data();
            imageMemBindInfos.emplace_back(imageMemoryBindInfo);
        }
    }
    bindInfo.imageOpaqueBindCount = opaqueBindInfos.size();
    bindInfo.pImageOpaqueBinds = opaqueBindInfos.data();
    bindInfo.imageBindCount = imageMemBindInfos.size();
    bindInfo.pImageBinds = imageMemBindInfos.data();

    std::vector<VkSemaphore> waits;
    if (!_waits.empty()) {
        for (auto* sem : _waits) {
            waits.emplace_back(sem->semaphore());
        }
        bindInfo.waitSemaphoreCount = waits.size();
        bindInfo.pWaitSemaphores = waits.data();
        _waits.clear();
    } else {
        bindInfo.waitSemaphoreCount = 0;
    }

    std::vector<VkSemaphore> signals;
    if (!_signals.empty()) {
        for (auto* sem : _signals) {
            signals.emplace_back(sem->semaphore());
        }
        bindInfo.signalSemaphoreCount = signals.size();
        bindInfo.pSignalSemaphores = signals.data();
        _signals.clear();
    } else {
        bindInfo.signalSemaphoreCount = 0;
    }

    VK_EXPECT(vkQueueBindSparse(_vkQueue, 1, &bindInfo, VK_NULL_HANDLE));

    _commandBuffers.clear();
}

void Queue::addWait(RHISemaphore* s) {
    if (s) {
        auto* sem = static_cast<Semaphore*>(s);
        _waits.emplace_back(sem);
    }
}

void Queue::addSignal(RHISemaphore* s) {
    if (s) {
        auto* sem = static_cast<Semaphore*>(s);
        _signals.emplace_back(sem);
    }
}

void Queue::addCompleteHandler(std::function<void()>&& func) {
    _completeHandlers[_currFrameIndex].emplace_back(std::forward<std::function<void()>>(func));
}

} // namespace raum::rhi
