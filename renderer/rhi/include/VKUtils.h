#pragma once
#include <vulkan/vulkan.h>
#include "log.h"
namespace raum {
template <>
void log(const std::vector<VkExtensionProperties>& vec) {
    for (const auto& ele : vec) {
        spdlog::log(spdlog::level::info, "{}. {}", &ele - &vec[0], ele.extensionName);
    }
}

template <>
void log(const std::vector<VkLayerProperties>& vec) {
    for (const auto& ele : vec) {
        spdlog::log(spdlog::level::info, "{}. {}", &ele - &vec[0], ele.layerName);
    }
}

} // namespace raum