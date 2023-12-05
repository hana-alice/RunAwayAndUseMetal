#include "VKDevice.h"
#include "VKUtils.h"
#include "utils/log.h"
namespace raum::rhi {

static constexpr bool enableValidationLayer{true};
VKDevice* VKDevice::s_inst = nullptr;

namespace {
bool checkRequiredLayers(const std::vector<const char*>& requires, const std::vector<VkLayerProperties>& availables) {
    bool found = false;
    for (const char* require : requires) {
        for (const auto& layer : availables) {
            if (strcmp(require, layer.layerName) == 0) {
                found = true;
                break;
            }
        }
    }
    return found;
}

bool checkRequiredExtensions(const std::vector<const char*> requires, const std::vector<VkExtensionProperties>& availables) {
    for (const char* require : requires) {
        bool found = false;
        for (const auto& layer : availables) {
            if (strcmp(require, layer.extensionName) == 0) {
                found = true;
                break;
            }
        }
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) {
    // if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    // } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    // } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    // } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    // }
    raum::log(pCallbackData->pMessage);

    return VK_FALSE;
}

VkResult createDebugMessengerExt(VkInstance instance,
                                 const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                 const VkAllocationCallbacks* pAllocator,
                                 VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void destroyDebugMessengerExt(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func) {
        func(instance, debugMessenger, pAllocator);
    }
}

uint32_t checkDevice(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    uint32_t score{1};
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 100;
    }
    if (deviceFeatures.geometryShader) {
        score += 100;
    }
    if (deviceFeatures.sparseBinding) {
        score += 100;
    }

    return score;
}

VkPhysicalDevice rankDevices(const std::vector<VkPhysicalDevice>& devices) {
    uint32_t maxScore{0};
    VkPhysicalDevice chosen;
    for (const auto& device : devices) {
        uint32_t score = checkDevice(device);
        if (score > maxScore) {
            maxScore = score;
            chosen = device;
        }
    }
    return chosen;
}

} // namespace

VKDevice* VKDevice::getInstance() {
    if (!s_inst) {
        s_inst = new VKDevice();
    }
    return s_inst;
}

VKDevice::VKDevice() {
    initInstance();
    initDevice();
}

VKDevice::~VKDevice() {
    if (enableValidationLayer) {
        destroyDebugMessengerExt(_instance, _debugMessenger, nullptr);
    }
    vkDestroyDevice(_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
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

    // extension
    VkResult result = VK_SUCCESS;
    {
        uint32_t extensionNum{0};
        result = vkEnumerateInstanceExtensionProperties(nullptr, &extensionNum, nullptr);
        RAUM_CRITICAL_IF(!extensionNum || result != VK_SUCCESS, "Failed to get vulkan instance extension properties");
        std::vector<VkExtensionProperties> availableExts(extensionNum);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionNum, &availableExts[0]);
        RAUM_CRITICAL_IF(result != VK_SUCCESS, "Failed to get vulkan instance extension properties");
        raum::log(availableExts);

        uint32_t layerNum{0};
        result = vkEnumerateInstanceLayerProperties(&layerNum, nullptr);
        RAUM_CRITICAL_IF(result != VK_SUCCESS, "vkEnumerateInstanceExtensionProperties");
        std::vector<VkLayerProperties> availableLayers(layerNum);
        result = vkEnumerateInstanceLayerProperties(&layerNum, availableLayers.data());
        raum::log(availableLayers);

        std::vector<const char*> requiredLayers;
        if constexpr (enableValidationLayer) {
            requiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
        }
        bool res = checkRequiredLayers(requiredLayers, availableLayers);
        RAUM_CRITICAL_IF(!res, "required layers not found!");

        VkInstanceCreateInfo instInfo{};
        instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instInfo.pApplicationInfo = &appInfo;

        std::vector<const char*> requiredExts;
        if constexpr (enableValidationLayer) {
            requiredExts.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

            VkDebugUtilsMessengerCreateInfoEXT dbgMsgInfo{};
            dbgMsgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            dbgMsgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            dbgMsgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            dbgMsgInfo.pfnUserCallback = debugCallback;
            dbgMsgInfo.pUserData = nullptr;

            // result = createDebugMessengerExt(_instance, &dbgMsgInfo, nullptr, &_debugMessenger);
            // RAUM_ERROR_IF(result == VK_ERROR_EXTENSION_NOT_PRESENT, "validation ext not found.");

            instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&dbgMsgInfo;
        }

        instInfo.enabledExtensionCount = requiredExts.size();
        instInfo.ppEnabledExtensionNames = requiredExts.data();
        instInfo.enabledLayerCount = requiredLayers.size();
        instInfo.ppEnabledLayerNames = requiredLayers.data();

        result = vkCreateInstance(&instInfo, nullptr, &_instance);
        RAUM_CRITICAL_IF(result != VK_SUCCESS, "vkCreateInstance");
    }
}

void VKDevice::initDevice() {
    uint32_t deviceCount{0};
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    RAUM_CRITICAL_IF(!deviceCount, "can't find any physic device.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    _physicalDevice = rankDevices(devices);

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

    uint32_t queueFamilyCount{0};
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &queueFamilyCount, queueFamilies.data());

    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            _indices.graphicsFamily.index = &queueFamily - &queueFamilies[0];
        } else if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            _indices.computeFamily.index = &queueFamily - &queueFamilies[0];
        } else if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            _indices.transferFamily.index = &queueFamily - &queueFamilies[0];
        }
    }

    RAUM_CRITICAL_IF(!_indices.graphicsFamily.index.has_value(), "Graphics queue not support.");

    float priority = 1.0f;
    queueInfo.queueFamilyIndex = _indices.graphicsFamily.index.value();
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.enabledExtensionCount = 0;
    deviceInfo.enabledLayerCount = 0;

    VkResult res = vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device);
    RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create logic device.");

    vkGetDeviceQueue(_device, _indices.graphicsFamily.index.value(), 0, &_indices.graphicsFamily.queue);
}

} // namespace raum::rhi
