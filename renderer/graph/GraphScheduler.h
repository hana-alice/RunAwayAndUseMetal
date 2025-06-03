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
    explicit GraphScheduler(
        rhi::DevicePtr device,
        rhi::SwapchainPtr swapchain,
        RenderGraph* _renderGraph,
        ResourceGraph* _resourceGraph,
        AccessGraph* _accessGraph,
        TaskGraph* _taskGraph,
        SceneGraph* _sceneGraph,
        ShaderGraph* _shaderGraph);

    void needWarmUp();
    void execute(rhi::CommandBufferPtr cmd);

private:
    RenderGraph* _renderGraph;
    TaskGraph* _taskGraph;
    AccessGraph* _accessGraph;
    SceneGraph* _sceneGraph;
    ShaderGraph* _shaderGraph;
    ResourceGraph* _resourceGraph;

    rhi::SwapchainPtr _swapchain;
    rhi::DevicePtr _device;

    bool _warmed{false};

    std::unordered_map<std::string, scene::BindGroupPtr, hash_string, std::equal_to<>> _perPhaseBindGroups;

    scene::BVHNode* _bvhRoot{nullptr};
    std::vector<scene::RenderablePtr> _renderables;
    std::span<scene::RenderablePtr> _noCullRenderables;
    std::span<scene::RenderablePtr> _cullableRenderables;

    scene::MeshRendererPtr _fullscreenMeshRenderer{nullptr};
};

using GraphSchedulerPtr = std::shared_ptr<GraphScheduler>;

} // namespace raum::graph
