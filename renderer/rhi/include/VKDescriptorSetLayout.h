#pragma once
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class DescriptorSetLayout {
public:
    DescriptorSetLayout() = delete;
    DescriptorSetLayout(const DescriptorSetLayout&) = delete;
    DescriptorSetLayout(DescriptorSetLayout&&) = delete;
    DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;

    explicit DescriptorSetLayout(const DescriptorSetLayoutInfo& info);
    ~DescriptorSetLayout();

    VkDescriptorSetLayout layout() const { return _descriptorSetLayout; }

private:
    VkDescriptorSetLayout _descriptorSetLayout;
};
}