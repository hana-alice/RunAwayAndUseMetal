#pragma once
#include "RHIComputePipeline.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class PipelineLayout;
class ComputePipeline : public RHIComputePipeline {
public:
    ComputePipeline() = delete;
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline(ComputePipeline&&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;

    explicit ComputePipeline(const ComputePipelineInfo& info, Device* device);
    ~ComputePipeline();

    VkPipeline pipeline() const { return _pipeline; }
    PipelineLayout* pipelineLayout() const { return _layout; }

private:
    Device* _device{nullptr};
    PipelineLayout* _layout{nullptr};
    VkPipeline _pipeline;
};
} // namespace raum::rhi