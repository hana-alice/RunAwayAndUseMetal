#include "VKDevice.h"
#include "utils/log.h"

namespace rhi {

VKDevice::VKDevice() {
}

VKDevice::~VKDevice() {
}

void VKDevice::initInstance() {
    // Vk App info
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Experimental";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "RAUM";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    //
    uint32_t extensionNum{0};
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionNum, nullptr);
    RAUM_CRITICAL_IF(!extensionNum, "Failed to get vulkan instance extension properties");

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
}

} // namespace rhi
