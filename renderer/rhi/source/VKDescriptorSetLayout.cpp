#include "VKDescriptorSetLayout.h"
#include "VKUtils.h"
#include "VKDevice.h"
namespace raum::rhi {
DescriptorSetLayout::DescriptorSetLayout(const DescriptorSetLayoutInfo& info) {
    VkDescriptorSetLayoutCreateInfo descLayoutInfo{};
    descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    std::vector<VkDescriptorSetLayoutBinding> bindings(info.descriptorBindings.size());
    for (size_t i = 0; i < info.descriptorBindings.size(); i++) {
        const auto& bindingInfo = info.descriptorBindings[i];
        bindings[i].binding = bindingInfo.binding;
        bindings[i].descriptorType = descriptorType(bindingInfo.type);
        bindings[i].descriptorCount = bindingInfo.count;
        bindings[i].stageFlags = shaderStageFlags(info.descriptorBindings[i].visibility);
        bindings[i].pImmutableSamplers = nullptr;
    }
    descLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    descLayoutInfo.pBindings = bindings.data();
    
    vkCreateDescriptorSetLayout(Device::getInstance()->device(), &descLayoutInfo, nullptr, &_descriptorSetLayout);
}

DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(Device::getInstance()->device(), _descriptorSetLayout, nullptr);
}

}
