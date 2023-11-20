#pragma once

#include <vulkan/vulkan.h>

namespace rhi {
class VKDevice {
public:
    static VKDevice *getInstance();

private:
    VKDevice();
    ~VKDevice();

    VKDevice(const VKDevice &) = delete;
    VKDevice(VKDevice &&);

    void initInstance();

    static VKDevice *s_inst;

    VkInstance _instance{nullptr};

    VkDebugUtilsMessengerEXT _debugMessenger;
};
} // namespace rhi