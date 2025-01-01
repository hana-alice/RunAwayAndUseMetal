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
    constexpr static uint32_t shadowMapWidth = 1024;
    constexpr static uint32_t shadowMapHeight = 1024;
    constexpr static Vec3f lightColor{1.0, 1.0, 1.0};

public:
    explicit ShadowMapSample(framework::Director* director) : _ppl(director->pipeline()), _director(director) {
    }

    void init() override {
        // load scene from gltf
        _device = _director->device();
        _swapchain = _director->swapchain();
        const auto& resourcePath = utils::resourceDirectory();

        // load models
        {
            auto& sceneGraph = _director->sceneGraph();
            asset::serialize::load(sceneGraph, resourcePath / "models" / "DamagedHelmet" / "DamagedHelmet.gltf", _device);

            auto& quad = asset::BuiltinRes::quad();
            graph::ModelNode& quadNode = sceneGraph.addModel("quad");
            quadNode.model = quad.model();
        }


        auto width = _swapchain->width();
        auto height = _swapchain->height();
        scene::PerspectiveFrustum frustum{45.0f, width / (float)height, 0.01f, 10.0f};
        _cam = std::make_shared<scene::Camera>(frustum);
        auto& eye = _cam->eye();
        eye.setPosition(0.0, 0.0f, 4.0);
        eye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        eye.update();

        scene::OrthoFrustum shadowFrustum{-5.0f, 5.0f, -5.0f, 5.0f, 0.01f, 10.0f};
        _shadowCam = std::make_shared<scene::Camera>(shadowFrustum);
        auto& shadowEye = _shadowCam->eye();
        shadowEye.setPosition(5.0, 5.0, 0.0);
        shadowEye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        shadowEye.update();

        auto& resourceGraph = _ppl->resourceGraph();
        if (!resourceGraph.contains(_forwardRT)) {
            resourceGraph.import(_forwardRT, _swapchain);
        }
        if (!resourceGraph.contains(_forwardDS)) {
            resourceGraph.addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, width, height, rhi::Format::D24_UNORM_S8_UINT);
        }
        if (!resourceGraph.contains(_shadowMapRT)) {
            resourceGraph.addImage(_shadowMapRT, rhi::ImageUsage::COLOR_ATTACHMENT | rhi::ImageUsage::SAMPLED, shadowMapWidth, shadowMapHeight, rhi::Format::R32_SFLOAT);
        }
        if (!resourceGraph.contains(_camBuffer)) {
            resourceGraph.addBuffer(_camBuffer, 128, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
            resourceGraph.addBuffer(_camPose, 12, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
            resourceGraph.addBuffer(_light, 32, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
        }


    }

    ~ShadowMapSample() {
        _keyListener.remove();
        _mouseListener.remove();
    }

    void show() override {
        auto& renderGraph = _ppl->renderGraph();
        _ppl->resourceGraph().updateImage("forwardDS", _swapchain->width(), _swapchain->height());

        // shadow buffer upload pass
        {
            auto uploadPass = renderGraph.addCopyPass("shadowCamUpdate");
            auto& shadowEye = _shadowCam->eye();
            const auto& shadowViewMat = shadowEye.inverseAttitude();
            uploadPass.uploadBuffer(&shadowViewMat[0], 64, _camBuffer, 0);
            const auto& shadowProjMat = shadowEye.projection();
            uploadPass.uploadBuffer(&shadowProjMat[0], 64, _camBuffer, 64);
        }

        // shadow rendering pass
        {
            auto shadowPass = renderGraph.addRenderPass("shadowMap");
            shadowPass.addColor(_shadowMapRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.0f});
            auto shadowQ = shadowPass.addQueue("shadowMap");
            shadowQ.setViewport(0, 0, shadowMapWidth, shadowMapHeight, 0.0f, 1.0f)
                   .addCamera(_shadowCam.get())
                   .addUniformBuffer(_camBuffer, "Mat");
        }

        // main camera upload pass
        {
            auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");
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
        }

        // rendering
        {
            auto renderPass = renderGraph.addRenderPass("forward");

            static  float a = 0.0f;
            a += 0.1f;
            renderPass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {std::sin(a) , 0.3, 0.3, 1.0})
                      .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::STORE, graph::LoadOp::CLEAR, graph::StoreOp::STORE, 1.0, 0);
            auto queue = renderPass.addQueue("solidColor");

            auto width = _swapchain->width();
            auto height = _swapchain->height();
            queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
                .addCamera(_cam.get())
                .addUniformBuffer(_camBuffer, "Mat");
                 //.addUniformBuffer(_camPose, "CamPos")
                 //.addUniformBuffer(_light, "Light");
        }

    }

    void hide() override {
        _director->sceneGraph().disable("ShadowMap");
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
    std::shared_ptr<scene::Camera> _shadowCam;
    std::shared_ptr<scene::Scene> _scene;

    const std::string _forwardRT = "forwardRT";
    const std::string _forwardDS = "forwardDS";
    const std::string _shadowMapRT = "shadowMap";
    const std::string _camBuffer = "camBuffer";
    const std::string _camPose = "camPose";
    const std::string _light = "light";

    const std::string _name = "Particles";

    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseButtonEventTag> _mouseListener;
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
};
} // namespace raum::sample