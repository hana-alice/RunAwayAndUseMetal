#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include "VKDefine.h"

namespace raum::rhi {
class VKQueue;
class VKDevice {
public:
    static VKDevice *getInstance();

    VkPhysicalDevice physicalDevice() { return _physicalDevice; };

private:
    VKDevice();
    ~VKDevice();

    VKDevice(const VKDevice &) = delete;
    VKDevice(VKDevice &&);

    void initInstance();
    void initDevice();

    static VKDevice *s_inst;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;

    QueueFamilyIndices _indices{};
};
} // namespace raum::rhi