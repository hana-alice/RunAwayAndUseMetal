#pragma once
#include "AccessGraph.h"
#include "RenderGraph.h"
#include "ResourceGraph.h"
#include "SceneGraph.h"
#include "ShaderGraph.h"
#include "TaskGraph.h"

namespace raum::graph {

class GraphScheduler {
public:
    GraphScheduler() = delete;
    explicit GraphScheduler(rhi::DevicePtr device, rhi::SwapchainPtr swapchain);

    void execute();

private:
    RenderGraph* _renderGraph;
    TaskGraph* _taskGraph;
    AccessGraph* _accessGraph;
    SceneGraph* _sceneGraph;
    ShaderGraph* _shaderGraph;
    ResourceGraph* _resourceGraph;

    std::unordered_map<rhi::RHIImageView*, rhi::CommandBufferPtr> _commandBuffers;
    rhi::CommandPoolPtr _commandPool;
    rhi::SwapchainPtr _swapchian;
    rhi::DevicePtr _device;

};

} // namespace raum::graph
