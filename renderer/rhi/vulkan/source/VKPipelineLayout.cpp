#include "VKPipelineLayout.h"
#include "VKDescriptorSetLayout.h"
#include "VKDevice.h"
#include "VKUtils.h"
namespace raum::rhi {

PipelineLayout::PipelineLayout(const PipelineLayoutInfo& info, RHIDevice* device)
: RHIPipelineLayout(info, device), _device(static_cast<Device*>(device)) {
    VkPipelineLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    std::vector<VkPushConstantRange> ranges(info.pushConstantRanges.size());
    for (size_t i = 0; i < info.pushConstantRanges.size(); ++i) {
        const auto& pRange = info.pushConstantRanges[i];
        ranges[i] = {shaderStageFlags(pRange.stage), pRange.offset, pRange.size};
    }

    std::vector<VkDescriptorSetLayout> layouts(info.setLayouts.size());
    for (size_t i = 0; i < info.setLayouts.size(); ++i) {
        auto* setLayout = static_cast<DescriptorSetLayout*>(info.setLayouts[i]);
        layouts[i] = setLayout->layout();
    }

    createInfo.pushConstantRangeCount = static_cast<uint32_t>(ranges.size());
    createInfo.pPushConstantRanges = ranges.data();
    createInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    createInfo.pSetLayouts = layouts.data();

    VkResult res = vkCreatePipelineLayout(_device->device(), &createInfo, nullptr, &_layout);
    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create pipeline layout");
}

PipelineLayout::~PipelineLayout() {
    vkDestroyPipelineLayout(_device->device(), _layout, nullptr);
}

} // namespace raum::rhi