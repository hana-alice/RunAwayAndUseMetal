#pragma once
#include "BuiltinRes.h"
#include "Camera.h"
#include "GraphScheduler.h"
#include "KeyboardEvent.h"
#include "Mesh.h"
#include "MouseEvent.h"
#include "PBRMaterial.h"
#include "RHIDevice.h"
#include "SceneSerializer.h"
#include "Serialization.h"
#include "WindowEvent.h"
#include "common.h"
#include "core/utils/utils.h"
#include "math.h"
#include "Director.h"
namespace raum::sample {
class ShadowMapSample : public SampleBase {
public:
    explicit ShadowMapSample(framework::Director* director) : _ppl(director->pipeline()), _director(director) {}

    void init() override {
        // load scene from gltf
        _device = _director->device();
        _swapchain = _director->swapchain();
        const auto& resourcePath = utils::resourceDirectory();
        auto& sceneGraph = _director->sceneGraph();
        // asset::serialize::load(sceneGraph, resourcePath / "models" / "DamagedHelmet" / "DamagedHelmet.gltf", _device);

        auto& skybox = asset::BuiltinRes::skybox();
        graph::ModelNode& skyboxNode = sceneGraph.addModel("skybox");
        skyboxNode.model = skybox.model();

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        scene::PerspectiveFrustum frustum{45.0f, width / (float)height, 0.01f, 10.0f};
        _cam = std::make_shared<scene::Camera>(frustum, scene::Projection::PERSPECTIVE);
        auto& eye = _cam->eye();
        eye.setPosition(0.0, 0.0f, 4.0);
        eye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        eye.update();

        auto& resourceGraph = _ppl->resourceGraph();
        if (!resourceGraph.contains(_forwardRT)) {
            resourceGraph.import(_forwardRT, _swapchain);
        }
        if (!resourceGraph.contains(_forwardDS)) {
            resourceGraph.addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, width, height, rhi::Format::D24_UNORM_S8_UINT);
        }
        if (!resourceGraph.contains(_computeRes)) {
            uint32_t size = width * height;
            resourceGraph.addBuffer(_computeRes, size, rhi::BufferUsage::STORAGE);
        }
        if (!resourceGraph.contains(_camBuffer)) {
            resourceGraph.addBuffer(_camBuffer, 128, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
            resourceGraph.addBuffer(_camPose, 12, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
            resourceGraph.addBuffer(_light, 32, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
        }

        auto mouseHandler = [&, width, height](int32_t x, int32_t y, framework::MouseButton btn, framework::ButtonStatus status) {

        };
        _mouseListener.add(mouseHandler);
    }

    ~ShadowMapSample() {
        _keyListener.remove();
        _mouseListener.remove();
    }

    void show() override {
        auto& renderGraph = _ppl->renderGraph();
        auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");

        _ppl->resourceGraph().updateImage("forwardDS", _swapchain->width(), _swapchain->height());

        auto& eye = _cam->eye();
        auto viewMat = eye.inverseAttitude();
        uploadPass.uploadBuffer(&viewMat[0], 64, _camBuffer, 0);
        const auto& projMat = eye.projection();
        uploadPass.uploadBuffer(&projMat[0], 64, _camBuffer, 64);

        uploadPass.uploadBuffer(&eye.getPosition()[0], 12, _camPose, 0);
        Vec4f color{1.0, 1.0, 1.0, 1.0};
        Vec4f lightPos{5.0, 5.0, 0.0, 1.0};
        uploadPass.uploadBuffer(&lightPos[0], 16, _light, 0);
        uploadPass.uploadBuffer(&color[0], 16, _light, 16);

        auto computePass = renderGraph.addComputePass("compute");
        computePass.setPhase("")
            .addResource(_computeRes, "", graph::Access::WRITE);

        auto renderPass = renderGraph.addRenderPass("forward");
        renderPass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.3, 0.3, 0.3, 1.0})
            .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::STORE, graph::LoadOp::CLEAR, graph::StoreOp::STORE, 1.0, 0);
        auto queue = renderPass.addQueue("default");

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
            .addCamera(_cam.get())
            .addUniformBuffer(_camBuffer, "Mat")
            .addUniformBuffer(_camPose, "CamPos")
            .addUniformBuffer(_light, "Light");
    }

    void hide() override {
        _director->sceneGraph().disable("Particles");
    }

    const std::string& name() override {
        return _name;
    }

private:
    graph::PipelinePtr _ppl;
    framework::Director* _director;

    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;

    graph::GraphSchedulerPtr _graphScheduler;

    std::shared_ptr<scene::Camera> _cam;
    std::shared_ptr<scene::Scene> _scene;

    const std::string _forwardRT = "forwardRT";
    const std::string _forwardDS = "forwardDS";
    const std::string _computeRes = "computeRes";
    const std::string _camBuffer = "camBuffer";
    const std::string _camPose = "camPose";
    const std::string _light = "light";

    const std::string _name = "Particles";

    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseEventTag> _mouseListener;
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
};

} // namespace raum::sample