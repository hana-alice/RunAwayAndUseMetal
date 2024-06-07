#pragma once
#include "RHIGraphicsPipeline.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class PipelineLayout;
class GraphicsPipeline : public RHIGraphicsPipeline {
public:
    GraphicsPipeline() = delete;
    GraphicsPipeline(const GraphicsPipeline&) = delete;
    GraphicsPipeline(GraphicsPipeline&&) = delete;
    GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    explicit GraphicsPipeline(const GraphicsPipelineInfo& info, Device* device);
    ~GraphicsPipeline();

    VkPipeline pipeline() const { return _pipeline; }
    PipelineLayout* pipelineLayout() const { return _layout; }

private:
    Device* _device{nullptr};
    PipelineLayout* _layout{nullptr};
    VkPipeline _pipeline;
};
} // namespace raum::rhi