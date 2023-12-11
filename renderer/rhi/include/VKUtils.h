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

VkCullModeFlags cullMode(CullMode mode);

VkPolygonMode polygonMode(PolygonMode mode);

VkFrontFace frontFace(FrontFace face);

VkSampleCountFlagBits sampleCount(uint32_t sampleCount);

VkCompareOp compareOp(CompareOp compareOp);

VkStencilOp stencilOp(StencilOp stencilOp);

VkBlendFactor blendFactor(BlendFactor blendFactor);

VkBlendOp blendOp(BlendOp blendOp);

VkColorComponentFlags colorComponentFlags(Channel channel);

VkLogicOp logicOp(LogicOp op);

} // namespace rhi

} // namespace raum