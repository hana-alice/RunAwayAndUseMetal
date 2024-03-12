#pragma once
#include "RenderGraph.h"
#include "ResourceGraph.h"
#include "SceneLoader.h"
#include "Serialization.h"
#include "ShaderGraph.h"
#include "common.h"
#include "core/utils/utils.h"
#include "RHIDevice.h"
#include "Camera.h"
namespace raum::sample {
class GraphSample : public SampleBase {
public:
    explicit GraphSample(rhi::DevicePtr device, rhi::SwapchainPtr swapchain);
    void show() override;

private:
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;
    std::shared_ptr<graph::ShaderGraph> _shaderGraph;
    std::shared_ptr<graph::RenderGraph> _renderGraph;
    std::shared_ptr<graph::ResourceGraph> _resourceGraph;

    std::shared_ptr<scene::Camera> _cam;
};

GraphSample::GraphSample(rhi::DevicePtr device, rhi::SwapchainPtr swapchain)
: _device(device), _swapchain(swapchain) {
    _shaderGraph = std::make_shared<graph::ShaderGraph>(_device.get());

    const auto& resourcePath = utils::resourceDirectory();

    graph::deserialize(resourcePath / "shader", "simple", *_shaderGraph);
    _shaderGraph->compile("asset");

    asset::SceneLoader loader(device);
    loader.load(resourcePath / "models" / "sponza-gltf-pbr" / "sponza.glb");

    const auto& imageInfo = swapchain->swapchainImageView()->image()->info();
    auto width = imageInfo.extent.x;
    auto height = imageInfo.extent.y;
    scene::Frustum frustum{45.0f, width / (float)height, 0.1, 1000.0};
    _cam = std::make_shared<scene::Camera>(frustum, scene::Projection::PERSPECTIVE);
}

void GraphSample::show() {

}

} // namespace raum::sample