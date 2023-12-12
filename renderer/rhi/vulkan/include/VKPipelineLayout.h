#pragma once
#include "VKDefine.h"
namespace raum::rhi {
class PipelineLayout {
public:
    PipelineLayout() = delete;
    PipelineLayout(const PipelineLayout&) = delete;
    PipelineLayout(PipelineLayout&&) = delete;
    PipelineLayout& operator=(const PipelineLayout&) = delete;

    explicit PipelineLayout(const PipelineLayoutInfo& info);
    ~PipelineLayout();

    VkPipelineLayout layout() const { return _layout; }

private:
    VkPipelineLayout _layout;
};
} // namespace raum::rhi
