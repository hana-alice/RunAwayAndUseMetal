#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <memory>
#include <queue>
#include "VKDefine.h"
namespace raum::rhi {
class Queue;
class Swapchain;
class Device {
public:
    static Device *getInstance();

    VkPhysicalDevice physicalDevice() { return _physicalDevice; };
    VkDevice device() { return _device; }
    VkInstance instance() { return _instance; }

    Queue *defaultQueue() { return _queues.at(QueueType::GRAPHICS); }

    Queue *createQueue(const QueueInfo &queueInfo);
    Swapchain *createSwapchain(const SwapchainInfo &info);

private:
    Device();
    ~Device();

    Device(const Device &) = delete;
    Device(Device &&);

    void initInstance();
    void initDevice();
    void initSurface(void *hwnd);

    static Device *s_inst;

    VkInstance _instance;
    VkSurfaceKHR _surface;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;

    std::map<QueueType, Queue *> _queues;
};
} // namespace raum::rhi