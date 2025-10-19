#pragma once
#include <vulkan/vulkan.h>
#include "core/core.h"
#include "VKDefine.h"
#include "core/utils/log.h"

namespace raum {
template <>
inline void log(const std::vector<VkExtensionProperties>& vec) {
    for (const auto& ele : vec) {
        spdlog::log(spdlog::level::info, "{}. {}", &ele - &vec[0], ele.extensionName);
    }
}

template <>
inline void log(const std::vector<VkLayerProperties>& vec) {
    for (const auto& ele : vec) {
        spdlog::log(spdlog::level::info, "{}. {}", &ele - &vec[0], ele.layerName);
    }
}

template<>
struct equalTo<VkPipelineBinaryKeyKHR> {
    bool operator()(const VkPipelineBinaryKeyKHR& lhs, const VkPipelineBinaryKeyKHR& rhs) const {
        if (lhs.sType != rhs.sType || lhs.keySize != rhs.keySize) return false;
        return std::memcmp(lhs.key, rhs.key, lhs.keySize) == 0;
    }
};

namespace rhi {
FormatInfo formatInfo(Format format);

VkVertexInputRate mapRate(InputRate rate);

VkCullModeFlags cullMode(FaceMode mode);

VkPolygonMode polygonMode(PolygonMode mode);

VkFrontFace frontFace(FrontFace face);

VkSampleCountFlagBits sampleCount(uint32_t sampleCount);

VkCompareOp compareOp(CompareOp compareOp);

VkStencilOp stencilOp(StencilOp stencilOp);

VkBlendFactor blendFactor(BlendFactor blendFactor);

VkBlendOp blendOp(BlendOp blendOp);

VkColorComponentFlags colorComponentFlags(Channel channel);

VkLogicOp logicOp(LogicOp op);

VkDescriptorType descriptorType(DescriptorType type);

VkShaderStageFlags shaderStageFlags(ShaderStage stage);

VkAttachmentLoadOp loadOp(LoadOp op);

VkAttachmentStoreOp storeOp(StoreOp op);

VkImageLayout imageLayout(ImageLayout layout);

VkPipelineStageFlags pipelineStageFlags(PipelineStage stage);

VkAccessFlags accessFlags(AccessFlags flags);

VkDependencyFlags dependencyFlags(DependencyFlags flags);

VkImageType imageType(ImageType type);

VkImageCreateFlags imageFlag(ImageFlag flag);

VkImageUsageFlags imageUsage(ImageUsage usage);

VkSharingMode sharingMode(SharingMode mode);

VkBufferCreateFlags bufferFlag(BufferFlag flag);

VkImageViewType viewType(ImageViewType viewType);

VkComponentSwizzle componentSwizzle(ComponentSwizzle component);

VkImageAspectFlags aspectMask(AspectMask mask);

VkCommandBufferLevel commandBufferLevel(CommandBufferType commandBufferLevel);

void fillClearColors(std::vector<VkClearValue>& clearValues,
                     ClearValue* colors,
                     const std::vector<AttachmentInfo>& attachmentInfos);

void fillClearAttachment(std::vector<VkClearAttachment>& clearAttachment,
                         ClearValue* colors,
                         uint32_t* attachmentIndices,
                         uint32_t attachmentNum,
                         const std::vector<AttachmentInfo>& attachmentInfos);

void fillClearRect(std::vector<VkClearRect>& clearRects,
                   ClearRect* rects,
                   uint32_t count);

void fillClearColors(std::vector<VkClearColorValue>& clearValues,
                     ClearValue* colors,
                     Format format);

VkStencilFaceFlags stencilFaceFlags(FaceMode faceMode);

VkIndexType indexType(IndexType indexType);

Format mapSwapchainFormat(VkFormat format);

VkCommandBufferUsageFlags commandBufferUsage(CommandBuferUsageFlag flag);

bool isDepthStencil(Format format);

VkPrimitiveTopology primitiveTopology(PrimitiveType primitiveType);

VkFilter mapFilter(Filter filter);

FormatType formatType(Format format);

VkSamplerMipmapMode samplerMipmapMode(MipmapMode mipmapMode);

VkSamplerAddressMode samplerAddressMode(SamplerAddressMode addressMode);

VkBorderColor borderColor(BorderColor borderColor);

VkPipelineBinaryKHR getOrCachePipelineBinary(VkDevice device, const VkGraphicsPipelineCreateInfo& info);

} // namespace rhi

} // namespace raum