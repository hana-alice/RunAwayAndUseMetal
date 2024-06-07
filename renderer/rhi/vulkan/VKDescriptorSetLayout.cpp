#include "VKDescriptorSetLayout.h"
#include "VKDevice.h"
#include "VKUtils.h"
#include "VKSampler.h"
namespace raum::rhi {
DescriptorSetLayout::DescriptorSetLayout(const DescriptorSetLayoutInfo& info, RHIDevice* device)
: RHIDescriptorSetLayout(info, device), _device(static_cast<Device*>(device)) {
    VkDescriptorSetLayoutCreateInfo descLayoutInfo{};
    descLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    std::vector<VkDescriptorSetLayoutBinding> bindings(info.descriptorBindings.size());
    for (size_t i = 0; i < info.descriptorBindings.size(); i++) {
        const auto& bindingInfo = info.descriptorBindings[i];
        bindings[i].binding = bindingInfo.binding;
        bindings[i].descriptorType = descriptorType(bindingInfo.type);
        bindings[i].descriptorCount = bindingInfo.count;
        bindings[i].stageFlags = shaderStageFlags(bindingInfo.visibility);
        std::vector<VkSampler> samplers(bindingInfo.immutableSamplers.size());
        for (size_t j = 0; j < bindingInfo.immutableSamplers.size(); j++) {
            samplers[j] = static_cast<const Sampler*>(bindingInfo.immutableSamplers[j])->sampler();
        }
        
        bindings[i].pImmutableSamplers = samplers.data();
    }
    descLayoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    descLayoutInfo.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(_device->device(), &descLayoutInfo, nullptr, &_descriptorSetLayout);
}

DescriptorSetLayout::~DescriptorSetLayout() {
    vkDestroyDescriptorSetLayout(_device->device(), _descriptorSetLayout, nullptr);
}

} // namespace raum::rhi
