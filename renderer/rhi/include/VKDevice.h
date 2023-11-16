#pragma once

#include <vulkan/vulkan.h>

namespace rhi {
class VKDevice {
public:
private:
    VKDevice();
    ~VKDevice();

    VKDevice(const VKDevice &) = delete;
    VKDevice(VKDevice &&);

    void initInstance();

    VkInstance _instance{nullptr};
};
} // namespace rhi