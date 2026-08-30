#include "VKDevice.h"
#include "RHIManager.h"
#include "VKBuffer.h"
#include "VKCache.h"
#include "VKCommandPool.h"
#include "VKComputePipeline.h"
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
#include "VKSemaphore.h"
#include "VKShader.h"
#include "VKSparseImage.h"
#include "VKSwapchain.h"
#include "VKUtils.h"
#include "VkBufferView.h"
#include "core/utils/log.h"
#include "core/utils/utils.h"
#include <memory>
#include <set>
#include <stdexcept>

// #include "asset/serialization/Archive.h"
namespace raum::rhi {

#if defined(NDEBUG)
static constexpr bool enableValidationLayer{false};
#else
static constexpr bool enableValidationLayer{true};
#endif

static constexpr uint32_t ChunkSize{1024 * 1024 * 4};

namespace {
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
    initCache();
}

void Device::initCache() {
    _programCache = new ProgramCache(this);
    _programCache->validate();
}

Device::~Device() {
    vkDeviceWaitIdle(_device);

    delete _programCache;

    for (auto& [_, sampler] : _samplers) {
        delete sampler;
    }

    for (auto& [_, stagingBuffer] : _stagingBuffers) {
        delete stagingBuffer;
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
    appInfo.apiVersion = VK_API_VERSION_1_4;

    const auto availableExts = VK_ENUMERATE(
        VkExtensionProperties, vkEnumerateInstanceExtensionProperties, nullptr);
    VK_ENSURE(!availableExts.empty(), "No Vulkan instance extensions are available");
    raum::log(availableExts);

    const auto availableLayers = VK_ENUMERATE(VkLayerProperties, vkEnumerateInstanceLayerProperties);
    raum::log(availableLayers);

    std::vector<const char*> requiredLayers;
    std::vector<const char*> requiredExts;
    if constexpr (enableValidationLayer) {
        requiredLayers.emplace_back("VK_LAYER_KHRONOS_validation");
        requiredExts.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
#ifdef RAUM_WINDOWS
    requiredExts.emplace_back(VK_KHR_SURFACE_EXTENSION_NAME);
    requiredExts.emplace_back("VK_KHR_win32_surface");
#endif

    requireVkProperties(requiredLayers, availableLayers, "layers");
    requireVkProperties(requiredExts, availableExts, "instance extensions");

    VkInstanceCreateInfo instInfo{};
    instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExts.size());
    instInfo.ppEnabledExtensionNames = requiredExts.data();
    instInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    instInfo.ppEnabledLayerNames = requiredLayers.data();

    VK_EXPECT(vkCreateInstance(&instInfo, nullptr, &_instance));

    if constexpr (enableValidationLayer) {
        VkDebugUtilsMessengerCreateInfoEXT dbgMsgInfo{};
        dbgMsgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dbgMsgInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dbgMsgInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        dbgMsgInfo.pfnUserCallback = debugCallback;
        dbgMsgInfo.pUserData = nullptr;

        VK_EXPECT(createDebugMessengerExt(_instance, &dbgMsgInfo, nullptr, &_debugMessenger));
    }
}

void Device::initDevice() {
    const auto devices = VK_ENUMERATE(VkPhysicalDevice, vkEnumeratePhysicalDevices, _instance);
    VK_ENSURE(!devices.empty(), "No Vulkan physical device is available");

    _physicalDevice = rankDevices(devices);

    auto addQueue = [this](QueueType type, bool required) {
        try {
            auto queue = std::unique_ptr<Queue>(new Queue(QueueInfo{type}, this));
            _queues.emplace(type, queue.release());
        } catch (const std::runtime_error&) {
            if (required) {
                throw;
            }
            raum_warn("Optional Vulkan queue type is not supported: {}", static_cast<uint32_t>(type));
        }
    };
    addQueue(QueueType::GRAPHICS, true);
    addQueue(QueueType::COMPUTE, false);
    addQueue(QueueType::TRANSFER, false);
    addQueue(QueueType::SPARSE, false);

    float priority = 1.0f;
    std::set<uint32_t> queueFamilies;
    for (const auto& [_, queue] : _queues) {
        queueFamilies.emplace(queue->_index);
    }
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    queueInfos.reserve(queueFamilies.size());
    for (const auto family : queueFamilies) {
        auto& queueInfo = queueInfos.emplace_back();
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = family;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &priority;
    }

    VkPhysicalDeviceFeatures2 deviceFeatures2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    auto& deviceFeatures = deviceFeatures2.features;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    const auto availableExts = VK_ENUMERATE(
        VkExtensionProperties, vkEnumerateDeviceExtensionProperties, _physicalDevice, nullptr);
    log(availableExts);

    auto isExtAvailable = [&](const char* name) {
        return hasVkProperty(availableExts, name);
    };

    const bool pipelineBinaryExtensionAvailable = isExtAvailable(VK_KHR_PIPELINE_BINARY_EXTENSION_NAME);

    // Query first and only enable features actually supported by the selected device.
    VkPhysicalDeviceFeatures2 supportedFeatures{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    VkPhysicalDeviceTimelineSemaphoreFeatures supportedTimelineFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR};
    VkPhysicalDevicePipelineBinaryFeaturesKHR supportedPipelineBinaryFeatures{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_FEATURES_KHR};
    supportedFeatures.pNext = &supportedTimelineFeatures;
    if (pipelineBinaryExtensionAvailable) {
        supportedTimelineFeatures.pNext = &supportedPipelineBinaryFeatures;
    }
    vkGetPhysicalDeviceFeatures2(_physicalDevice, &supportedFeatures);

    VK_ENSURE(supportedTimelineFeatures.timelineSemaphore,
              "The selected Vulkan device does not support timeline semaphores");

    const bool hasSparseQueue = _queues.contains(QueueType::SPARSE);
    const bool sparseResidencySupported = hasSparseQueue &&
                                          supportedFeatures.features.sparseBinding &&
                                          supportedFeatures.features.sparseResidencyImage2D &&
                                          supportedFeatures.features.shaderResourceResidency;
    deviceFeatures.sparseBinding = sparseResidencySupported;
    deviceFeatures.sparseResidencyImage2D = sparseResidencySupported;
    deviceFeatures.shaderResourceResidency = sparseResidencySupported;
    if (hasSparseQueue && !sparseResidencySupported) {
        delete _queues.at(QueueType::SPARSE);
        _queues.erase(QueueType::SPARSE);
        raum_warn("Sparse image residency is not supported by the selected Vulkan device");
    }

    // All feature structs must outlive vkCreateDevice.
    VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
    timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
    timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;
    timelineSemaphoreFeatures.pNext = nullptr;

    VkPhysicalDevicePipelineBinaryFeaturesKHR pipelineBinaryFeatures{};

    std::vector<const char*> exts{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
        VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME,
    };

    if (pipelineBinaryExtensionAvailable && supportedPipelineBinaryFeatures.pipelineBinaries) {
        exts.emplace_back(VK_KHR_PIPELINE_BINARY_EXTENSION_NAME);

        pipelineBinaryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_BINARY_FEATURES_KHR;
        pipelineBinaryFeatures.pipelineBinaries = VK_TRUE;
        pipelineBinaryFeatures.pNext = &timelineSemaphoreFeatures;
        deviceFeatures2.pNext = &pipelineBinaryFeatures;
    } else {
        deviceFeatures2.pNext = &timelineSemaphoreFeatures;
    }

    requireVkProperties(exts, availableExts, "device extensions");

    deviceInfo.pNext = &deviceFeatures2;
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size());
    deviceInfo.pEnabledFeatures = nullptr;
    deviceInfo.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    deviceInfo.ppEnabledExtensionNames = exts.data();
    deviceInfo.enabledLayerCount = 0;

    VK_EXPECT(vkCreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device));

    VmaAllocatorCreateInfo allocInfo{};
    allocInfo.device = _device;
    allocInfo.physicalDevice = _physicalDevice;
    allocInfo.instance = _instance;
    allocInfo.vulkanApiVersion = VK_API_VERSION_1_4;
    VK_EXPECT(vmaCreateAllocator(&allocInfo, &_allocator));

    for (auto [_, q] : _queues) {
        vkGetDeviceQueue(_device, q->_index, 0, &q->_vkQueue);
        q->initQueue();
    }

    auto* gpk = vkGetDeviceProcAddr(_device, "vkGetPipelineKeyKHR");
    pfn_vkGetPipelineKeyKHR = reinterpret_cast<PFN_vkGetPipelineKeyKHR>(gpk);

    if (!pfn_vkGetPipelineKeyKHR) {
        raum_warn("Couldn't get pfn_vkGetPipelineKeyKHR");
    }
}

RHIQueue* Device::getQueue(const QueueInfo& info) {
    const auto iter = _queues.find(info.type);
    VK_ENSURE(iter != _queues.end(), "Requested Vulkan queue type is not supported");
    return iter->second;
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
    return new Shader(info, this);
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
    VK_ENSURE(_queues.contains(QueueType::SPARSE), "Sparse images are not supported by the selected Vulkan device");
    return new SparseImage(info, this);
}

RHISemaphore* Device::createSemaphore() {
    return new Semaphore(this);
}

SparseBindingRequirement Device::sparseBindingRequirement(RHIImage* image) {
    VK_ENSURE(test(image->info().imageFlag, rhi::ImageFlag::SPARSE_BINDING), "The image does not support sparse binding");
    const auto vkImage = dynamic_cast<SparseImage*>(image)
        ? static_cast<SparseImage*>(image)->image()
        : static_cast<Image*>(image)->image();
    std::vector<VkSparseImageMemoryRequirements> reqs;
    uint32_t count{0};
    vkGetImageSparseMemoryRequirements(_device, vkImage, &count, nullptr);
    VK_ENSURE(count, "The image has no sparse memory requirements");
    reqs.resize(count);
    vkGetImageSparseMemoryRequirements(_device, vkImage, &count, reqs.data());

    SparseBindingRequirement req{};
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
    req.mipTailOffset = reqs[0].imageMipTailOffset;
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
    VK_EXPECT(vkDeviceWaitIdle(_device));
}

void Device::waitQueueIdle(raum::rhi::RHIQueue* q) {
    auto* queue = static_cast<Queue*>(q);
    VK_EXPECT(vkQueueWaitIdle(queue->_vkQueue));
    queue->completePendingHandlers();
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
