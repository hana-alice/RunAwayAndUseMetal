#include "VKQueue.h"
#include <optional>
#include <vector>
#include "VKDevice.h"
#include "log.h"
namespace raum::rhi {
Queue::Queue(const QueueInfo& info, Device* device) : RHIQueue(info, device) {
    _info = info;

    uint32_t queueFamilyCount{0};
    auto physicDevice = device->physicalDevice();
    vkGetPhysicalDeviceQueueFamilyProperties(physicDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicDevice, &queueFamilyCount, queueFamilies.data());

    std::optional<uint32_t> index;
    for (const auto& queueFamily : queueFamilies) {
        if (_info.type == QueueType::GRAPHICS && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            index = &queueFamily - &queueFamilies[0];
            break;
        } else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            index = &queueFamily - &queueFamilies[0];
            break;
        } else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            index = &queueFamily - &queueFamilies[0];
            break;
        }
    }

    RAUM_CRITICAL_IF(!index.has_value(), "Queue type not support.");
    _index = static_cast<uint32_t>(index.value());
}
} // namespace raum::rhi
