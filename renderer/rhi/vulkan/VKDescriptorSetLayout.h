#pragma once
#include "RHIDescriptorSetLayout.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class DescriptorSetLayout : public RHIDescriptorSetLayout {
public:
    DescriptorSetLayout() = delete;
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout(DescriptorSetLayout&&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    explicit DescriptorSetLayout(const DescriptorSetLayoutInfo& info, RHIDevice* device);
    ~DescriptorSetLayout();

    VkDescriptorSetLayout layout() const { return _descriptorSetLayout; }

private:
    Device* _device{nullptr};
    VkDescriptorSetLayout _descriptorSetLayout;
};
} // namespace raum::rhi