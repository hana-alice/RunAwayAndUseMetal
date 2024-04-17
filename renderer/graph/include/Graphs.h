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

    //deprecated futrue
    RenderGraph& renderGraph() { return *_renderGraph; }
    SceneGraph& sceneGraph() { return *_sceneGraph; }
    ShaderGraph& shaderGraph() { return *_shaderGraph; }
    ResourceGraph& resourceGraph() { return *_resourceGraph; }
    rhi::CommandPoolPtr commandPool() { return _commandPool; }

private:
    RenderGraph* _renderGraph;
    TaskGraph* _taskGraph;
    AccessGraph* _accessGraph;
    SceneGraph* _sceneGraph;
    ShaderGraph* _shaderGraph;
    ResourceGraph* _resourceGraph;

    std::vector<rhi::CommandBufferPtr> _commandBuffers;
    rhi::CommandPoolPtr _commandPool;
    rhi::SwapchainPtr _swapchain;
    rhi::DevicePtr _device;

    std::unordered_map<std::string, scene::BindGroupPtr> _perPhaseBindGroups;
};

} // namespace raum::graph
