#pragma once
#include "Graphs.h"
#include "SceneLoader.h"
#include "Serialization.h"
#include "common.h"
#include "core/utils/utils.h"
#include "RHIDevice.h"
#include "Camera.h"
#include "PBRMaterial.h"
#include "math.h"
namespace raum::sample {
class GraphSample : public SampleBase {
public:
    explicit GraphSample(rhi::DevicePtr device, rhi::SwapchainPtr swapchain);
    void show() override;

private:
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;

    graph::GraphScheduler* _graphScheduler;

    std::shared_ptr<scene::Camera> _cam;
    std::shared_ptr<scene::Scene> _scene;

    const std::string _forwardRT = "forwardRT";
    const std::string _forwardDS = "forwardDS";
    const std::string _camBuffer = "camBuffer";
    const std::string _camPose = "camPose";
    const std::string _light = "light";
};

GraphSample::GraphSample(rhi::DevicePtr device, rhi::SwapchainPtr swapchain)
: _device(device), _swapchain(swapchain) {
    _graphScheduler = new graph::GraphScheduler(device, swapchain);
    auto& shaderGraph = _graphScheduler->shaderGraph();

    const auto& resourcePath = utils::resourceDirectory();

    graph::deserialize(resourcePath / "shader", "cook-torrance_mesh", shaderGraph);
    shaderGraph.compile("asset");

    asset::SceneLoader loader(device);
    loader.loadFlat(resourcePath / "models" / "sponza-gltf-pbr" / "sponza.glb", scene::MeshType::MESH);
    auto model = loader.modelData();
    for(auto& meshRenderer : model->meshRenderers()) {
        auto pbrMat = std::static_pointer_cast<raum::scene::PBRMaterial>(meshRenderer->technique(0)->material());
        pbrMat->setDiffuse("mainTexture");
    }
    auto& sceneGraph = _graphScheduler->sceneGraph();
    auto& modelNode = sceneGraph.addModel("sponza", "");
    modelNode.model = model;

    const auto &aabb = model->aabb();
    auto far = std::abs(aabb.maxBound.z);

    auto width = _swapchain->width();
    auto height = _swapchain->height();
    scene::Frustum frustum{45.0f, width / (float)height, 0.1, 10 * far};
    _cam = std::make_shared<scene::Camera>(frustum, scene::Projection::PERSPECTIVE);
    auto& eye = _cam->eye();
    eye.setPosition(0.0f, 50.0f, 0.0);
    eye.lookAt({100.0, 50.0, 0.0}, {0.0f, 1.0f, 0.0f});

    auto& resourceGraph = _graphScheduler->resourceGraph();
    if(!resourceGraph.contains(_forwardRT)) {
//        _resourceGraph->addImage(_forwardRT, rhi::ImageUsage::COLOR_ATTACHMENT, width, height, rhi::Format::BGRA8_UNORM);
        resourceGraph.import(_forwardRT, _swapchain);
    }
    if(!resourceGraph.contains(_forwardDS)) {
        resourceGraph.addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, width, height, rhi::Format::D24_UNORM_S8_UINT);
    }
    if(!resourceGraph.contains(_camBuffer)) {
        resourceGraph.addBuffer(_camBuffer, 192, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
        resourceGraph.addBuffer(_camPose, 12, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
        resourceGraph.addBuffer(_light, 32, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
    }
    
}

void GraphSample::show() {
    auto& renderGraph = _graphScheduler->renderGraph();
    auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");

    auto& eye = _cam->eye();
//    eye.translate(1.0, 0.0,  0.0);
    auto modelMat = Mat4(1.0);
    uploadPass.uploadBuffer(&modelMat[0], 64, _camBuffer, 0);
    auto viewMat = eye.attitude();
    uploadPass.uploadBuffer(&viewMat[0], 64, _camBuffer, 64);
    const auto& projMat = eye.projection();
    uploadPass.uploadBuffer(&projMat[0], 64, _camBuffer, 128);

    uploadPass.uploadBuffer(&eye.getPosition()[0], 12, _camPose, 0);
    Vec4f color{1.0, 1.0, 1.0, 1.0};
    Vec4f lightPos{10.0, 100.0, 0.0, 1.0};
    uploadPass.uploadBuffer(&lightPos[0], 16, _light, 0);
    uploadPass.uploadBuffer(&color[0], 16, _light, 16);

    auto renderPass = renderGraph.addRenderPass("forward");
    renderPass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.8, 0.1, 0.3, 1.0})
        .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::STORE, graph::LoadOp::CLEAR, graph::StoreOp::STORE, 1.0, 0);
    auto queue = renderPass.addQueue("default");

    auto width = _swapchain->width();
    auto height = _swapchain->height();
    queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
        .addCamera(_cam.get())
        .addUniformBuffer(_camBuffer, "Mat")
        .addUniformBuffer(_camPose, "CamPos")
        .addUniformBuffer(_light, "Light");

    _graphScheduler->execute();

}

} // namespace raum::sample