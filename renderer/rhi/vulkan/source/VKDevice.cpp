#include "VKDevice.h"
#include "RHIManager.h"
#include "VKBuffer.h"
#include "VKDescriptorSet.h"
#include "VKDescriptorSetLayout.h"
#include "VKGraphicsPipeline.h"
#include "VKQueue.h"
#include "VKRenderPass.h"
#include "VKSampler.h"
#include "VKShader.h"
#include "VKSwapchain.h"
#include "VKUtils.h"
#include "utils/log.h"
#include "VKImage.h"
#include "VKImageView.h"
namespace raum::rhi {

static constexpr bool enableValidationLayer{true};

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
Device::Device() {
    initInstance();
    initDevice();
}

Device::~Device() {
    if (enableValidationLayer) {
        destroyDebugMessengerExt(_instance, _debugMessenger, nullptr);
    }

    for (auto [_, q] : _queues) {
        delete q;
    }
    _queues.clear();

    vmaDestroyAllocator(_allocator);
    vkDestroyDevice(_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

void Device::initInstance() {
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
#ifdef WINDOWS
        requiredExts.emplace_back("VK_KHR_surface");
        requiredExts.emplace_back("VK_KHR_win32_surface");
#endif

        instInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExts.size());
        instInfo.ppEnabledExtensionNames = requiredExts.data();
        instInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
        instInfo.ppEnabledLayerNames = requiredLayers.data();

        result = vkCreateInstance(&instInfo, nullptr, &_instance);
        RAUM_CRITICAL_IF(result != VK_SUCCESS, "vkCreateInstance");
    }
}

void Device::initDevice() {
    uint32_t deviceCount{0};
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    RAUM_CRITICAL_IF(!deviceCount, "can't find any physic device.");

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

    _physicalDevice = rankDevices(devices);

    // for further use
    auto* queue = new Queue(QueueInfo{QueueType::COMPUTE}, this);
    _queues.emplace(QueueType::COMPUTE, queue);
    queue = new Queue(QueueInfo{QueueType::TRANSFER}, this);
    _queues.emplace(QueueType::TRANSFER, queue);
    queue = new Queue(QueueInfo{QueueType::GRAPHICS}, this);
    _queues.emplace(QueueType::GRAPHICS, queue);

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

    float priority = 1.0f;
    queueInfo.queueFamilyIndex = queue->_index;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    std::vector<const char*> exts{};
    exts.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    uint32_t extNum{0};
    vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extNum, nullptr);
    std::vector<VkExtensionProperties> availableExts(extNum);
    vkEnumerateDeviceExtensionProperties(_physicalDevice, nullptr, &extNum, availableExts.data());
    log(availableExts);

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = exts.data();
    deviceInfo.enabledLayerCount = 0;

    VkResult res = vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device);
    RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create logic device.");

    vkGetDeviceQueue(_device, queue->_index, 0, &queue->_vkQueue);

    VmaAllocatorCreateInfo allocInfo{};
    allocInfo.device = _device;
    allocInfo.physicalDevice = _physicalDevice;
    allocInfo.instance = _instance;
    allocInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    vmaCreateAllocator(&allocInfo, &_allocator);

    queue->initCommandQueue();
}

RHIQueue* Device::getQueue(const QueueInfo& info) {
    return _queues.at(info.type);
}

RHISwapchain* Device::createSwapchain(const SwapchainInfo& info) {
    return new Swapchain(info, this);
}

RHIBuffer* Device::createBuffer(const BufferInfo& info) {
    return new Buffer(info, this);
}

RHIBuffer* Device::createBuffer(const BufferSourceInfo& info) {
    return new Buffer(info, this);
}

RHIImage* Device::createImage(const ImageInfo& info) {
    return new Image(info, this);
}

RHIImageView* Device::createImageView(const ImageViewInfo& info) {
    return new ImageView(info, this);
}

RHIDescriptorSet* Device::createDescriptorSet(const DescriptorSetInfo& info) {
    return nullptr;
}

RHIDescriptorSetLayout* Device::createDescriptorSetLayout(const DescriptorSetLayoutInfo& info) {
    return new DescriptorSetLayout(info, this);
}

RHIShader* Device::createShader(const ShaderSourceInfo& info) {
    return new Shader(info, this);
}

RHIShader* Device::createShader(const ShaderBinaryInfo& info) {
    return nullptr;
}

RHIGraphicsPipeline* Device::createGraphicsPipeline(const GraphicsPipelineInfo& info) {
    return new GraphicsPipeline(info, this);
}

RHISampler* Device::getSampler(const SamplerInfo& info) {
    return nullptr;
}

RHIRenderPass* Device::createRenderPass(const RenderPassInfo& info) {
    return new RenderPass(info, this);
}

Device* loadRHI() {
    return new Device();
}

RHIDevice* loadRHI(API api) {
    return loadRHI();
}

} // namespace raum::rhi
