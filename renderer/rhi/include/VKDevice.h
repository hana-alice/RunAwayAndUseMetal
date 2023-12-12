#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <memory>
#include <queue>
#include "VKDefine.h"
#include "vk_mem_alloc.h"
namespace raum::rhi {
class Queue;
class Swapchain;
class Device {
public:
    static Device *getInstance();

    VkPhysicalDevice physicalDevice() { return _physicalDevice; };
    VkDevice device() { return _device; }
    VkInstance instance() { return _instance; }
    VmaAllocator &allocator() { return _allocator; }

    Queue *defaultQueue() { return _queues.at(QueueType::GRAPHICS); }

    Queue *createQueue(const QueueInfo &queueInfo);
    Swapchain *createSwapchain(const SwapchainInfo &info);

    Buffer *createBuffer(const BufferInfo &info);
    Buffer *createBuffer(const BufferSourceInfo &info);

private:
    Device();
    ~Device();

    Device(const Device &) = delete;
    Device(Device &&);

    void initInstance();
    void initDevice();

    static Device *s_inst;

    VkInstance _instance;
    VkSurfaceKHR _surface;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VmaAllocator _allocator;

    std::map<QueueType, Queue *> _queues;
};
} // namespace raum::rhi