//
// Created by zeqia on 2024/7/12.
//

#include "Pipeline.h"
#include "GraphUtils.h"

namespace raum::graph {

Pipeline::Pipeline(rhi::DevicePtr device, rhi::SwapchainPtr swapchain, graph::SceneGraphPtr sg, graph::ShaderGraphPtr shg)
: _device(device), _swapchain(swapchain), _sceneGraph(sg), _shaderGraph(shg) {
    _renderGraph = new RenderGraph(_device);
    _resourceGraph = new ResourceGraph(_device.get());
    _accessGraph = new AccessGraph(*_renderGraph, *_resourceGraph, *_shaderGraph);
    _taskGraph = new TaskGraph();
    _scheduler = new GraphScheduler(device, swapchain, _renderGraph, _resourceGraph, _accessGraph, _taskGraph, _sceneGraph.get(), _shaderGraph.get());
}

void Pipeline::run(rhi::CommandBufferPtr cmd) {
    _scheduler->execute(cmd);
}

void Pipeline::resizeSwapchain(uint32_t width, uint32_t height, uintptr_t surface) {
    if (!width || !height) {
        return;
    }

    _device->waitDeviceIdle();
    clearFrameBufferCache();
    _swapchain->resize(width, height, surface);
    _scheduler->needWarmUp();
}

Pipeline::~Pipeline() {
    delete _scheduler;
    delete _taskGraph;
    delete _accessGraph;
    delete _resourceGraph;
    delete _renderGraph;
}

} // namespace raum::graph
