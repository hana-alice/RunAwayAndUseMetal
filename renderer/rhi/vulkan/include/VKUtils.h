#pragma once
#include <vulkan/vulkan.h>
#include "VKDefine.h"
#include "log.h"
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
} // namespace rhi

} // namespace raum