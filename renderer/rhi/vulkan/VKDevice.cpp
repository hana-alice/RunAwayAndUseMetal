#include "VKDevice.h"
#include "RHIManager.h"
#include "VKBuffer.h"
#include "VKCommandPool.h"
#include "VKDescriptorPool.h"
#include "VKDescriptorSet.h"
#include "VKDescriptorSetLayout.h"
#include "VKFrameBuffer.h"
#include "VKGraphicsPipeline.h"
#include "VKImage.h"
#include "VKImageView.h"
#include "VKPipelineLayout.h"
#include "VKQueue.h"
#include "VKRenderPass.h"
#include "VKSampler.h"
#include "VKShader.h"
#include "VKSwapchain.h"
#include "VKSparseImage.h"
#include "VKUtils.h"
#include "VkBufferView.h"
#include "VKComputePipeline.h"
#include "core/utils/log.h"
namespace raum::rhi {

static constexpr bool enableValidationLayer{true};

static constexpr uint32_t ChunkSize{1024 * 1024 * 4};

namespace {
bool checkRequiredLayers(const std::vector<const char*>& reqs, const std::vector<VkLayerProperties>& availables) {
    bool found = false;
    for (const char* require : reqs) {
        for (const auto& layer : availables) {
            if (strcmp(require, layer.layerName) == 0) {
                found = true;
                break;
            }
        }
    }
    return found;
}

bool checkRequiredExtensions(const std::vector<const char*> reqs, const std::vector<VkExtensionProperties>& availables) {
    for (const char* require : reqs) {
        bool found = false;
        for (const auto& layer : availables) {
            if (strcmp(require, layer.extensionName) == 0) {
                found = true;
                break;
            }
        }
    }
    return false;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) {
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    } else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        raum_error("{}", pCallbackData->pMessage);
    }

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
    vkDeviceWaitIdle(_device);

    for (auto& [_, sampler] : _samplers) {
        delete sampler;
    }

    for (auto [_, q] : _queues) {
        delete q;
    }

    vmaDestroyAllocator(_allocator);

    if (enableValidationLayer) {
        destroyDebugMessengerExt(_instance, _debugMessenger, nullptr);
    }

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
        }
#ifdef RAUM_WINDOWS
        requiredExts.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
        requiredExts.emplace_back("VK_KHR_win32_surface");
#endif

        instInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExts.size());
        instInfo.ppEnabledExtensionNames = requiredExts.data();
        instInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
        instInfo.ppEnabledLayerNames = requiredLayers.data();

        result = vkCreateInstance(&instInfo, nullptr, &_instance);
        RAUM_CRITICAL_IF(result != VK_SUCCESS, "vkCreateInstance");

        if constexpr (enableValidationLayer) {
            VkDebugUtilsMessengerCreateInfoEXT dbgMsgInfo{};
            dbgMsgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            dbgMsgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            dbgMsgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            dbgMsgInfo.pfnUserCallback = debugCallback;
            dbgMsgInfo.pUserData = nullptr;

            result = createDebugMessengerExt(_instance, &dbgMsgInfo, nullptr, &_debugMessenger);
            RAUM_ERROR_IF(result == VK_ERROR_EXTENSION_NOT_PRESENT, "validation ext not found.");

            instInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&dbgMsgInfo;
        }
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
    queue = new Queue(QueueInfo{QueueType::SPARSE}, this);
    _queues.emplace(QueueType::SPARSE, queue);
    queue = new Queue(QueueInfo{QueueType::GRAPHICS}, this);
    _queues.emplace(QueueType::GRAPHICS, queue);

    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;

    float priority = 1.0f;
    queueInfo.queueFamilyIndex = queue->_index;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.sparseBinding = 1;
    deviceFeatures.sparseResidencyImage2D = 1;
    deviceFeatures.shaderResourceResidency = 1;

    std::vector<const char*> exts{};
    exts.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    exts.emplace_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);

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
    deviceInfo.enabledExtensionCount = exts.size();
    deviceInfo.ppEnabledExtensionNames = exts.data();
    deviceInfo.enabledLayerCount = 0;

    VkResult res = vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device);
    RAUM_CRITICAL_IF(res != VK_SUCCESS, "failed to create logic device.");

    //vkGetDeviceQueue(_device, queue->_index, 0, &queue->_vkQueue);

    VmaAllocatorCreateInfo allocInfo{};
    allocInfo.device = _device;
    allocInfo.physicalDevice = _physicalDevice;
    allocInfo.instance = _instance;
    allocInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    vmaCreateAllocator(&allocInfo, &_allocator);

    for (auto [_, q] : _queues) {
        vkGetDeviceQueue(_device, q->_index, 0, &q->_vkQueue);
        q->initQueue();
    }
}

RHIQueue* Device::getQueue(const QueueInfo& info) {
    return _queues.at(info.type);
}

RHISwapchain* Device::createSwapchain(const SwapchainInfo& info) {
    return new Swapchain(info, this);
}

RHISwapchain* Device::createSwapchain(const SwapchainSurfaceInfo& info) {
    return new Swapchain(info, this);
}

RHIBuffer* Device::createBuffer(const BufferInfo& info) {
    return new Buffer(info, this);
}

RHIBuffer* Device::createBuffer(const BufferSourceInfo& info) {
    return new Buffer(info, this);
}

RHIBufferView* Device::createBufferView(const BufferViewInfo& info) {
    return new BufferView(info, this);
}

RHIImage* Device::createImage(const ImageInfo& info) {
    return new Image(info, this);
}

RHIImageView* Device::createImageView(const ImageViewInfo& info) {
    return new ImageView(info, this);
}

RHICommandPool* Device::createCoomandPool(const CommandPoolInfo& info) {
    return new CommandPool(info, this);
}

RHIDescriptorPool* Device::createDescriptorPool(const DescriptorPoolInfo& info) {
    return new DescriptorPool(info, this);
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

RHIComputePipeline* Device::createComputePipeline(const ComputePipelineInfo& info) {
    return new ComputePipeline(info, this);
}

RHISampler* Device::getSampler(const SamplerInfo& info) {
    if (_samplers.find(info) == _samplers.end()) {
        _samplers[info] = new Sampler(info, this);
    }
    return _samplers.at(info);
}

RHIRenderPass* Device::createRenderPass(const RenderPassInfo& info) {
    return new RenderPass(info, this);
}

RHIFrameBuffer* Device::createFrameBuffer(const FrameBufferInfo& info) {
    return new FrameBuffer(info, this);
}

RHIPipelineLayout* Device::createPipelineLayout(const PipelineLayoutInfo& info) {
    return new PipelineLayout(info, this);
}

RHISparseImage* Device::createSparseImage(const raum::rhi::SparseImageInfo& info) {
    return new SparseImage(info, this);
}

SparseBindingRequirement Device::sparseBindingRequirement(RHIImage* image) {
    auto* img = static_cast<Image*>(image);
    raum_check(test(image->info().imageFlag, rhi::ImageFlag::SPARSE_BINDING), "not a sparse image!");
    std::vector<VkSparseImageMemoryRequirements> reqs;
    VkMemoryRequirements memoryRequirements;
    uint32_t count;
    vkGetImageSparseMemoryRequirements(_device, img->image(), &count, nullptr);
    reqs.resize(count);
    vkGetImageSparseMemoryRequirements(_device, img->image(), &count, reqs.data());
    vkGetImageMemoryRequirements(_device, img->image(), &memoryRequirements);

    SparseBindingRequirement req;
    switch (reqs[0].formatProperties.aspectMask) {
        case VK_IMAGE_ASPECT_COLOR_BIT:
            req.aspect = AspectMask::COLOR;
            break;
        case VK_IMAGE_ASPECT_DEPTH_BIT:
            req.aspect = AspectMask::DEPTH;
            break;
        case VK_IMAGE_ASPECT_STENCIL_BIT:
            req.aspect = AspectMask::STENCIL;
            break;
        case VK_IMAGE_ASPECT_METADATA_BIT:
            req.aspect = AspectMask::METADATA;
            break;
        case VK_IMAGE_ASPECT_PLANE_0_BIT:
            req.aspect = AspectMask::PLANE_0;
            break;
        case VK_IMAGE_ASPECT_PLANE_1_BIT:
            req.aspect = AspectMask::PLANE_1;
            break;
        case VK_IMAGE_ASPECT_PLANE_2_BIT:
            req.aspect = AspectMask::PLANE_2;
            break;
    }
    req.granularity = {
        reqs[0].formatProperties.imageGranularity.width,
        reqs[0].formatProperties.imageGranularity.height,
        reqs[0].formatProperties.imageGranularity.depth,
    };
    if (reqs[0].formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_SINGLE_MIPTAIL_BIT) {
        req.flag |= SparseImageFormatFlag::SINGLE_MIPTAIL;
    }
    if (reqs[0].formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_ALIGNED_MIP_SIZE_BIT) {
        req.flag |= SparseImageFormatFlag::ALIGNED_MIP_SIZE;
    }
    if (reqs[0].formatProperties.flags & VK_SPARSE_IMAGE_FORMAT_NONSTANDARD_BLOCK_SIZE_BIT) {
        req.flag |= SparseImageFormatFlag::NONSTANDARD_BLOCK_SIZE;
    }
    req.mipTailFirstLod = reqs[0].imageMipTailFirstLod;
    req.mipTailSize = reqs[0].imageMipTailSize;
    req.mipTailOffset = reqs[0].imageMipTailSize;
    req.mipTailStride = reqs[0].imageMipTailStride;
    return req;
}

StagingBufferInfo Device::allocateStagingBuffer(uint32_t size, uint8_t queueIndex) {
    if (!_stagingBuffers.contains(queueIndex)) {
        _stagingBuffers.emplace(queueIndex, new RHIStagingBuffer(ChunkSize, this));
    }
    return _stagingBuffers.at(queueIndex)->allocate(size);
}

void Device::resetStagingBuffer(uint8_t queueIndex) {
    if (_stagingBuffers.contains(queueIndex)) {
        _stagingBuffers.at(queueIndex)->reset();
    }
}


void Device::waitDeviceIdle() {
    vkDeviceWaitIdle(_device);
}

void Device::waitQueueIdle(raum::rhi::RHIQueue* q) {
    auto* queue = static_cast<Queue*>(q);
    vkQueueWaitIdle(queue->_vkQueue);
}

Device* loadVK() {
    return new Device();
}

void unloadVK(Device* device) {
    delete device;
}

RHIDevice* loadRHI(API api) {
    return loadVK();
}

void unloadRHI(RHIDevice* device) {
    unloadVK(static_cast<Device*>(device));
}

} // namespace raum::rhi
