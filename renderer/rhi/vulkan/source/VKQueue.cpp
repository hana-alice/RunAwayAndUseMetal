#include "VKQueue.h"
#include <optional>
#include <vector>
#include "VKCommandBuffer.h"
#include "VKDevice.h"
#include "log.h"
namespace raum::rhi {
Queue::Queue(const QueueInfo& info, Device* device)
: RHIQueue(info, device), _device(static_cast<Device*>(device)) {
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
    vkDestroyCommandPool(_device->device(), _commandPool, nullptr);
}

RHICommandBuffer* Queue::makeCommandBuffer(const CommandBufferInfo& info) {
    return new CommandBuffer(info, this, _device);
}

void Queue::initPresentQueue(uint32_t presentCount) {
    _presentSemaphores.resize(presentCount);
    for (auto& sem : _presentSemaphores) {
        VkSemaphoreCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(_device->device(), &info, nullptr, &sem);
    }
}

void Queue::initCommandQueue() {
    VkCommandPoolCreateInfo cmdPoolInfo{};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = _index;
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(_device->device(), &cmdPoolInfo, nullptr, &_commandPool);

    _renderingSemaphores.resize(FRAMES_IN_FLIGHT);
    for (auto& sem : _renderingSemaphores) {
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
    VkPipelineStageFlags dstStage{};

    std::vector<VkCommandBuffer> cmdBuffers(_commandBuffers.size());
    for (size_t i = 0; i < _commandBuffers.size(); ++i) {
        cmdBuffers[i] = _commandBuffers[i]->commandBuffer();
    }
    info.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
    info.pCommandBuffers = cmdBuffers.data();

    if (!_presentSemaphores.empty()) {
        auto swapchainCount = static_cast<uint32_t>(_presentSemaphores.size());
        info.waitSemaphoreCount = 1;
        info.pWaitSemaphores = &_presentSemaphores[_currFrameIndex % swapchainCount];
        dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        info.pWaitDstStageMask = &dstStage;
        info.commandBufferCount = static_cast<uint32_t>(_commandBuffers.size());
        info.signalSemaphoreCount = 1;
        info.pSignalSemaphores = &_renderingSemaphores[_currFrameIndex];
    }

    VkFence lastFence = _frameFence[(_currFrameIndex - 1 + FRAMES_IN_FLIGHT) % FRAMES_IN_FLIGHT];
    vkQueueSubmit(_vkQueue, 1, &info, lastFence);
    vkWaitForFences(_device->device(), 1, &lastFence, VK_TRUE, UINT64_MAX);
    vkResetFences(_device->device(), 1, &_frameFence[_currFrameIndex]);

    _commandBuffers.clear();
}

void Queue::end() {
    _currFrameIndex = (_currFrameIndex + 1) % FRAMES_IN_FLIGHT;
}

VkSemaphore Queue::presentSemaphore() {
    RAUM_CRITICAL_IF(_presentSemaphores.empty(), "Trying to get a present semaphore while queue is not the presentation queue; or swapchain not init");

    auto swapchainCount = static_cast<uint32_t>(_presentSemaphores.size());
    return _presentSemaphores[_currFrameIndex % swapchainCount];
}

VkSemaphore Queue::renderSemaphore() {
    RAUM_CRITICAL_IF(_presentSemaphores.empty(), "Trying to get a render semaphore while queue is not the presentation queue; or swapchain not init");

    return _renderingSemaphores[_currFrameIndex % FRAMES_IN_FLIGHT];
}

} // namespace raum::rhi
