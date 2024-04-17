#include "VKQueue.h"
#include <optional>
#include <vector>
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "VKUtils.h"
namespace raum::rhi {
Queue::Queue(const QueueInfo& info, Device* device)
: RHIQueue(info, device), 
    _device(static_cast<Device*>(device)), 
    _currCommandSemaphore(VK_NULL_HANDLE){
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
        } else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            index = static_cast<uint32_t>(i);
            break;
        } else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            index = static_cast<uint32_t>(i);
            break;
        }
    }

    // vs warning
    if (index.has_value()) {
        _index = index.value();
    } else {
        RAUM_CRITICAL_IF(!index.has_value(), "Queue type not support.");
    }
}

Queue::~Queue() {
    if (_vkQueue != VK_NULL_HANDLE) {
        vkQueueWaitIdle(_vkQueue);
        for (auto sem : _commandSemaphores) {
            vkDestroySemaphore(_device->device(), sem, nullptr);
        }
        for (auto fence : _frameFence) {
            vkDestroyFence(_device->device(), fence, nullptr);
        }
    }
}

void Queue::initPresentQueue(uint32_t presentCount) {
    _presentSemaphores.resize(presentCount, VK_NULL_HANDLE);
}

void Queue::initCommandQueue() {
    _commandSemaphores.resize(FRAMES_IN_FLIGHT);
    for (auto& sem : _commandSemaphores) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(_device->device(), &info, nullptr, &sem);
    }

    _frameFence.resize(FRAMES_IN_FLIGHT);
    for (auto& fence : _frameFence) {
        VkFenceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vkCreateFence(_device->device(), &info, nullptr, &fence);
    }
}

void Queue::enqueue(RHICommandBuffer* cmdBuffer) {
    _commandBuffers.emplace_back(static_cast<CommandBuffer*>(cmdBuffer));
}

void Queue::submit() {
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
    auto presentSem = _presentSemaphores[_currFrameIndex];
    if (presentSem != VK_NULL_HANDLE) {
        waitSems.emplace_back(presentSem);
        waitStages.emplace_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
        _presentSemaphores[_currFrameIndex] = VK_NULL_HANDLE;
    }
    auto preTaskSem = popCommandSemaphore();
    if(preTaskSem != VK_NULL_HANDLE) {
        waitStages.emplace_back(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        waitSems.emplace_back(preTaskSem);
    }
    info.waitSemaphoreCount = waitSems.size();
    info.pWaitSemaphores = waitSems.data();
    info.pWaitDstStageMask = waitStages.data();
    info.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
    info.signalSemaphoreCount = 1;
    info.pSignalSemaphores = &_commandSemaphores[_currFrameIndex];
    _currCommandSemaphore = _commandSemaphores[_currFrameIndex];

    VkFence lastFence = _frameFence[(_currFrameIndex - 1 + FRAMES_IN_FLIGHT) % FRAMES_IN_FLIGHT];
    vkQueueSubmit(_vkQueue, 1, &info, lastFence);
    vkWaitForFences(_device->device(), 1, &lastFence, VK_TRUE, UINT64_MAX);
    vkResetFences(_device->device(), 1, &_frameFence[_currFrameIndex]);

    _currFrameIndex = (_currFrameIndex + 1) % FRAMES_IN_FLIGHT;
    for(auto& completeFunc : _completeHandlers[_currFrameIndex]) {
        completeFunc();
    }
    _completeHandlers[_currFrameIndex].clear();
    _commandBuffers.clear();
}

void Queue::setPresentSemaphore(VkSemaphore semaphore) {
    _presentSemaphores[_currFrameIndex] = semaphore;
}

VkSemaphore Queue::popCommandSemaphore() {
    VkSemaphore res = _currCommandSemaphore;
    _currCommandSemaphore = VK_NULL_HANDLE;
    return res;
}

void Queue::addCompleteHandler(const std::function<void()>& func) {
    _completeHandlers[_currFrameIndex].emplace_back(func);
}

} // namespace raum::rhi
