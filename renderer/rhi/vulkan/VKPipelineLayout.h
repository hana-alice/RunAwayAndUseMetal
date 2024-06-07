#pragma once
#include "RHIPipelineLayout.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class PipelineLayout : public RHIPipelineLayout {
public:
    PipelineLayout() = delete;
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout(PipelineLayout&&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;

    explicit PipelineLayout(const PipelineLayoutInfo& info, RHIDevice* device);
    ~PipelineLayout();

    VkPipelineLayout layout() const { return _layout; }

private:
    Device* _device{nullptr};
    VkPipelineLayout _layout;
};
} // namespace raum::rhi
