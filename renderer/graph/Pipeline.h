//
// Created by zeqia on 2024/7/12.
//

#pragma once
#include "GraphScheduler.h"
#include "RHIDefine.h"
namespace raum::graph {

class Pipeline {
public:
    explicit Pipeline(
        rhi::DevicePtr device,
        rhi::SwapchainPtr swapchain,
        graph::SceneGraphPtr sg,
        graph::ShaderGraphPtr shg);

    Pipeline(const Pipeline&) = delete;
    ~Pipeline();

    void run(rhi::CommandBufferPtr cmd);

    //    bool contains();

    ResourceGraph& resourceGraph() const { return *_resourceGraph; }
    RenderGraph& renderGraph() const { return *_renderGraph; }
    ShaderGraph& shaderGraph() const { return *_shaderGraph; };
    SceneGraph& sceneGraph() const { return *_sceneGraph; }
    GraphScheduler& graphScheduler() const { return *_scheduler; }

    rhi::DevicePtr device() const { return _device; }
    rhi::SwapchainPtr swapchain() const { return _swapchain; }

private:
    SceneGraphPtr _sceneGraph;
    ShaderGraphPtr _shaderGraph;
    RenderGraph* _renderGraph;
    ResourceGraph* _resourceGraph;
    AccessGraph* _accessGraph;
    TaskGraph* _taskGraph;
    GraphScheduler* _scheduler;
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;
};

using PipelinePtr = std::shared_ptr<Pipeline>;

} // namespace raum::graph
