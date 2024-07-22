//
// Created by zeqia on 2024/7/12.
//

#include "Pipeline.h"

namespace raum::graph {

Pipeline::Pipeline(rhi::DevicePtr device, rhi::SwapchainPtr swapchain, graph::SceneGraphPtr sg, graph::ShaderGraphPtr shg)
: _device(device), _swapchain(swapchain), _sceneGraph(sg), _shaderGraph(shg) {
    _renderGraph = new RenderGraph();
    _resourceGraph = new ResourceGraph(_device.get());
    _accessGraph = new AccessGraph(*_renderGraph, *_resourceGraph, *_shaderGraph);
    _scheduler = new GraphScheduler(device, swapchain, _renderGraph, _resourceGraph, _accessGraph, _taskGraph, _sceneGraph.get(), _shaderGraph.get());
}

void Pipeline::run(rhi::CommandBufferPtr cmd) {
    _scheduler->execute(cmd);
}

Pipeline::~Pipeline() {
    delete _renderGraph;
    delete _resourceGraph;
    delete _accessGraph;
    delete _taskGraph;
    delete _scheduler;
}

} // namespace raum::graph