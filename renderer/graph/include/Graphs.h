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
    explicit GraphScheduler(rhi::DevicePtr device);

    void execute();

private:
    rhi::DevicePtr _device;
    RenderGraph* _renderGraph;
    TaskGraph* _taskGraph;
    AccessGraph* _accessGraph;
    SceneGraph* _sceneGraph;
    ShaderGraph* _shaderGraph;
    ResourceGraph* _resourceGraph;
};

} // namespace raum::graph
