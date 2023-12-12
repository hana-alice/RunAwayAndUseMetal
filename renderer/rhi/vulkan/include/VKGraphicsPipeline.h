#pragma once
#include "VKDefine.h"

namespace raum::rhi {
class GraphicsPipelineState {
public:
    GraphicsPipelineState() = delete;
    GraphicsPipelineState(const GraphicsPipelineState&) = delete;
    GraphicsPipelineState(GraphicsPipelineState&&) = delete;
    GraphicsPipelineState& operator=(const GraphicsPipelineState&) = delete;
    
    explicit GraphicsPipelineState(const GraphicsPipelineInfo& info);
    ~GraphicsPipelineState();

    VkPipeline pipeline() const { return _pipeline; }

private:
    VkPipeline _pipeline;
};
} // namespace raum::rhi