#pragma once

#include <vulkan/vulkan.h>
#include <map>
#include <memory>
#include <queue>
#include "RHIDevice.h"
#include "vk_mem_alloc.h"
namespace raum::rhi {
class Queue;
class Swapchain;
class Buffer;
class Device : public RHIDevice {
public:
    VkPhysicalDevice physicalDevice() { return _physicalDevice; };
    VkDevice device() { return _device; }
    VkInstance instance() { return _instance; }
    VmaAllocator &allocator() { return _allocator; }

    RHISwapchain *createSwapchain(const SwapchainInfo &) override;
    RHIQueue *getQueue(const QueueInfo &) override;
    RHIBuffer *createBuffer(const BufferInfo &) override;
    RHIImage *createImage(const ImageInfo &) override;
    RHIImageView *createImageView(const ImageViewInfo &) override;
    RHISampler *getSampler(const SamplerInfo &) override;
    RHIShader *createShader(const ShaderBinaryInfo &) override;
    RHIShader *createShader(const ShaderSourceInfo &) override;
    RHIDescriptorSet *createDescriptorSet(const DescriptorSetInfo &) override;
    RHIDescriptorSetLayout *createDescriptorSetLayout(const DescriptorSetLayoutInfo &) override;
    RHIGraphicsPipeline *createGraphicsPipeline(const GraphicsPipelineInfo &) override;
    RHIRenderPass *createRenderPass(const RenderPassInfo &) override;
    RHIFrameBuffer *createFrameBuffer(const FrameBufferInfo &) override;
    RHIPipelineLayout *createPipelineLayout(const PipelineLayoutInfo &) override;

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

    friend Device *loadVK();
    friend void unloadVK(Device *);
};
} // namespace raum::rhi