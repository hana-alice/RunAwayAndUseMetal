#pragma once
#include "RenderGraph.h"
#include "ResourceGraph.h"
#include "Graphs.h"
#include "SceneLoader.h"
#include "Serialization.h"
#include "ShaderGraph.h"
#include "common.h"
#include "core/utils/utils.h"
#include "RHIDevice.h"
#include "Camera.h"
#include "PBRMaterial.h"
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
    std::shared_ptr<scene::Scene> _scene;

    const std::string _forwardRT = "forwardRT";
    const std::string _forwardDS = "forwardDS";
};

GraphSample::GraphSample(rhi::DevicePtr device, rhi::SwapchainPtr swapchain)
: _device(device), _swapchain(swapchain) {
    _shaderGraph = std::make_shared<graph::ShaderGraph>(_device);

    const auto& resourcePath = utils::resourceDirectory();

    graph::deserialize(resourcePath / "shader", "simple", *_shaderGraph);
    _shaderGraph->compile("asset");

    asset::SceneLoader loader(device);
    loader.loadFlat(resourcePath / "models" / "sponza-gltf-pbr" / "sponza.glb");
    const auto& model = loader.modelData();
    for(auto& meshRenderer : model->meshRenderers()) {
        auto pbrMat = std::static_pointer_cast<raum::scene::PBRMaterial>(meshRenderer->technique(0)->material());
        pbrMat->setDiffuse("mainTexture");
    }

    const auto &aabb = model->aabb();
    auto far = std::abs(aabb.maxBound.z);

    const auto& imageInfo = swapchain->swapchainImageView()->image()->info();
    auto width = imageInfo.extent.x;
    auto height = imageInfo.extent.y;
    scene::Frustum frustum{45.0f, width / (float)height, 0.1, 2 * far};
    _cam = std::make_shared<scene::Camera>(frustum, scene::Projection::PERSPECTIVE);
    auto& eye = _cam->eye();
    eye.setPosition(0.0f, 1.0f, far * 0.5f);
    eye.lookAt({}, {0.0f, 1.0f, 0.0f});

    _renderGraph = std::make_shared<graph::RenderGraph>();

    _resourceGraph = std::make_shared<graph::ResourceGraph>(_device.get());
    if(!_resourceGraph->contains(_forwardRT)) {
        _resourceGraph->addImage(_forwardRT, rhi::ImageUsage::COLOR_ATTACHMENT, width, height, rhi::Format::BGRA8_UNORM);
    }
    if(!_resourceGraph->contains(_forwardDS)) {
        _resourceGraph->addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, width, height, rhi::Format::D24_UNORM_S8_UINT);
    }
    
}

void GraphSample::show() {
    _resourceGraph->mount(_forwardRT);
    _resourceGraph->mount(_forwardDS);

    auto renderPass = _renderGraph->addRenderPass("forward");
    renderPass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.8, 0.1, 0.3, 1.0})
        .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::STORE, graph::LoadOp::CLEAR, graph::StoreOp::STORE, 1.0, 0);
    auto queue = renderPass.addQueue("");

    const auto& imageInfo = _swapchain->swapchainImageView()->image()->info();
    auto width = imageInfo.extent.x;
    auto height = imageInfo.extent.y;
    queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
        .addCamera(_cam.get());



//
//    _resourceGraph->unmount(_forwardRT);
//    _resourceGraph->unmount(_forwardDS);
}

} // namespace raum::sample