#include "VKPipelineLayout.h"
#include "VKUtils.h"
#include "VKDescriptorSetLayout.h"
#include "VKDevice.h"
#include "log.h"
namespace raum::rhi {

PipelineLayout::PipelineLayout(const PipelineLayoutInfo& info) {
    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    std::vector<VkPushConstantRange> ranges(info.pushConstantRanges.size());
    for (size_t i = 0; i < info.pushConstantRanges.size(); ++i) {
        const auto& pRange = info.pushConstantRanges[i];
        ranges[i] = {shaderStageFlags(pRange.stage), pRange.offset, pRange.size};
    }

    std::vector<VkDescriptorSetLayout> layouts(info.setLayouts.size());
    for (size_t i = 0; i < info.setLayouts.size(); ++i) {
        layouts[i] = info.setLayouts[i]->layout();
    }

    createInfo.pushConstantRangeCount = static_cast<uint32_t>(ranges.size());
    createInfo.pPushConstantRanges = ranges.data();
    createInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    createInfo.pSetLayouts = layouts.data();

    VkResult res = vkCreatePipelineLayout(Device::getInstance()->device(), &createInfo, nullptr, &_layout);
    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create pipeline layout");
}

PipelineLayout::~PipelineLayout() {
    vkDestroyPipelineLayout(Device::getInstance()->device(), _layout, nullptr);
}

} // namespace raum::rhi