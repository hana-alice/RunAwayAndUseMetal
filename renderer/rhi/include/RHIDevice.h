#pragma once
#include "RHIBuffer.h"
#include "RHICommandPool.h"
#include "RHIDescriptorPool.h"
#include "RHIDescriptorSet.h"
#include "RHIDescriptorSetLayout.h"
#include "RHIFrameBuffer.h"
#include "RHIGraphicsPipeline.h"
#include "RHIImage.h"
#include "RHIImageView.h"
#include "RHIPipelineLayout.h"
#include "RHIQueue.h"
#include "RHIRenderPass.h"
#include "RHISampler.h"
#include "RHIShader.h"
#include "RHISwapchain.h"
namespace raum::rhi {

class RHIDevice {
public:
    virtual RHISwapchain* createSwapchain(const SwapchainInfo&) = 0;
    virtual RHIBuffer* createBuffer(const BufferInfo&) = 0;
    virtual RHIBuffer* createBuffer(const BufferSourceInfo&) = 0;
    virtual RHIBufferView* createBufferView(const BufferViewInfo&) = 0;
    virtual RHIImage* createImage(const ImageInfo&) = 0;
    virtual RHIImageView* createImageView(const ImageViewInfo&) = 0;
    virtual RHIShader* createShader(const ShaderBinaryInfo&) = 0;
    virtual RHIShader* createShader(const ShaderSourceInfo&) = 0;
    virtual RHIDescriptorPool* createDescriptorPool(const DescriptorPoolInfo&) = 0;
    virtual RHICommandPool* createCoomandPool(const CommandPoolInfo&) = 0;
    virtual RHIDescriptorSetLayout* createDescriptorSetLayout(const DescriptorSetLayoutInfo&) = 0;
    virtual RHIPipelineLayout* createPipelineLayout(const PipelineLayoutInfo&) = 0;
    virtual RHIGraphicsPipeline* createGraphicsPipeline(const GraphicsPipelineInfo&) = 0;
    virtual RHIRenderPass* createRenderPass(const RenderPassInfo&) = 0;
    virtual RHIFrameBuffer* createFrameBuffer(const FrameBufferInfo&) = 0;

    // internal holds
    virtual RHIQueue* getQueue(const QueueInfo&) = 0;
    virtual RHISampler* getSampler(const SamplerInfo&) = 0;

protected:
    virtual ~RHIDevice() = 0;
};
inline RHIDevice::~RHIDevice() {}

} // namespace raum::rhi