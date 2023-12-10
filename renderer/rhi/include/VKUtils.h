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

} // namespace rhi

} // namespace raum