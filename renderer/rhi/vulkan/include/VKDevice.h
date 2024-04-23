#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <memory>
#include <queue>
#include <unordered_map>
#include "RHIDevice.h"
#include "vk_mem_alloc.h"
namespace raum::rhi {
class Queue;
class Swapchain;
class Buffer;
class Sampler;
class Device : public RHIDevice {
public:
    VkPhysicalDevice physicalDevice() { return _physicalDevice; };
    VkDevice device() { return _device; }
    VkInstance instance() { return _instance; }
    VmaAllocator &allocator() { return _allocator; }

    RHISwapchain *createSwapchain(const SwapchainInfo &) override;
    RHIQueue *getQueue(const QueueInfo &) override;
    RHIBuffer *createBuffer(const BufferInfo &) override;
    RHIBuffer *createBuffer(const BufferSourceInfo &) override;
    RHIBufferView *createBufferView(const BufferViewInfo &) override;
    RHIImage *createImage(const ImageInfo &) override;
    RHIImageView *createImageView(const ImageViewInfo &) override;
    RHISampler *getSampler(const SamplerInfo &) override;
    RHIShader *createShader(const ShaderBinaryInfo &) override;
    RHIShader *createShader(const ShaderSourceInfo &) override;
    RHIDescriptorSetLayout *createDescriptorSetLayout(const DescriptorSetLayoutInfo &) override;
    RHIGraphicsPipeline *createGraphicsPipeline(const GraphicsPipelineInfo &) override;
    RHIRenderPass *createRenderPass(const RenderPassInfo &) override;
    RHIFrameBuffer *createFrameBuffer(const FrameBufferInfo &) override;
    RHIPipelineLayout *createPipelineLayout(const PipelineLayoutInfo &) override;
    RHICommandPool *createCoomandPool(const CommandPoolInfo &);
    RHIDescriptorPool *createDescriptorPool(const DescriptorPoolInfo &) override;

    PFN_vkCmdDrawMeshTasksNV drawMeshTasksFunc();

private:
    Device();
    ~Device();

    Device(const Device &) = delete;
    Device(Device &&);

    void initInstance();
    void initDevice();

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VmaAllocator _allocator;

    std::map<QueueType, Queue *> _queues;
    std::vector<VkDescriptorPool> _descriptorPools;
    std::unordered_map<SamplerInfo, Sampler *, RHIHash<SamplerInfo>> _samplers;

    friend Device *loadVK();
    friend void unloadVK(Device *);
};
} // namespace raum::rhi