#pragma once
#include "VKDefine.h"

namespace raum::rhi {
class GraphicsPipelineState {
public:
    GraphicsPipelineState() = delete;
    GraphicsPipelineState(const GraphicsPipelineState&) = delete;
    GraphicsPipelineState(GraphicsPipelineState&&) = delete;
    GraphicsPipelineState& operator=(const GraphicsPipelineState&) = delete;
    explicit GraphicsPipelineState(const GraphicsPipelineStateInfo& info);

private:
};
} // namespace raum::rhi