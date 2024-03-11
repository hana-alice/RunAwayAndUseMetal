#pragma once
#include "RenderGraph.h"
#include "ResourceGraph.h"
#include "SceneLoader.h"
#include "Serialization.h"
#include "ShaderGraph.h"
#include "common.h"
#include "utils.h"


namespace raum::sample {
class GraphSample : public SampleBase {
public:
    explicit GraphSample(std::shared_ptr<RHIDevice> device, std::shared_ptr<RHISwapchain> swapchain);
    void show() override;

private:
    std::shared_ptr<RHIDevice> _device;
    std::shared_ptr<RHISwapchain> _swapchain;
    std::shared_ptr<graph::ShaderGraph> _shaderGraph;
    std::shared_ptr<graph::RenderGraph> _renderGraph;
    std::shared_ptr<graph::ResourceGraph> _resourceGraph;
};

GraphSample::GraphSample(std::shared_ptr<RHIDevice> device, std::shared_ptr<RHISwapchain> swapchain)
: _device(device), _swapchain(swapchain) {
    _shaderGraph = std::make_shared<graph::ShaderGraph>(_device.get());

    const auto& resourcePath = utils::resourceDirectory();

    graph::deserialize(resourcePath / "shader", "simple", *_shaderGraph);
    _shaderGraph->compile("asset");

    framework::asset::SceneLoader loader(device);
    loader.load(resourcePath / "models" / "sponza-gltf-pbr" / "sponza.glb");
}

void GraphSample::show() {
}

} // namespace raum::sample