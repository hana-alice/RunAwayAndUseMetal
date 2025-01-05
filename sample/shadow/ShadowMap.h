#pragma once
#include "BuiltinRes.h"
#include "Camera.h"
#include "Director.h"
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
            auto& quadRenderer = quadNode.model->meshRenderers().front();

            auto scaleMat = glm::scale(Mat4{1.0f}, Vec3f{3.0f, 3.0f, 3.0f});
            auto rotMat = glm::rotate(Mat4{1.0f}, glm::radians(-90.0f), Vec3f{1.0f, 0.0f, 0.0f});
            auto transMat = glm::translate(Mat4{1.0f}, Vec3f{0.0f, -1.0f, 0.0f});
            auto mat = transMat * rotMat * scaleMat;
            quadRenderer->setTransform(mat);
        }

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        scene::PerspectiveFrustum frustum{45.0f, width / (float)height, 0.1f, 50.0f};
        _cam = std::make_shared<scene::Camera>(frustum);
        auto& eye = _cam->eye();
        eye.setPosition(0.0, 0.0f, 4.0);
        eye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        eye.update();

        scene::OrthoFrustum shadowFrustum{-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 20.0f};
        _shadowCam = std::make_shared<scene::Camera>(shadowFrustum);
        auto& shadowEye = _shadowCam->eye();
        shadowEye.setPosition(5.0, 5.0, 5.0);
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
        if (!resourceGraph.contains(_shadowMapDS)) {
            resourceGraph.addImage(_shadowMapDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, shadowMapWidth, shadowMapHeight, rhi::Format::D24_UNORM_S8_UINT);
        }
        if (!resourceGraph.contains(_camBuffer)) {
            resourceGraph.addBuffer(_camBuffer, 128, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
            resourceGraph.addBuffer(_camPose, 12, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
            resourceGraph.addBuffer(_light, 32, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
            resourceGraph.addBuffer(_shadowVPBuffer, 136, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
        }
        if (!resourceGraph.contains(_shadowSampler)) {
            rhi::SamplerInfo info{};
            info.magFilter = rhi::Filter::LINEAR;
            info.minFilter = rhi::Filter::LINEAR;
            info.mipmapMode = rhi::MipmapMode::NEAREST;
            info.addressModeU = rhi::SamplerAddressMode::CLAMP_TO_EDGE;
            info.addressModeV = rhi::SamplerAddressMode::CLAMP_TO_EDGE;
            info.addressModeW = rhi::SamplerAddressMode::CLAMP_TO_EDGE;
            resourceGraph.addSampler(_shadowSampler, info);
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
            uploadPass.uploadBuffer(&shadowViewMat[0], 64, _shadowVPBuffer, 0);
            const auto& shadowProjMat = shadowEye.projection();
            uploadPass.uploadBuffer(&shadowProjMat[0], 64, _shadowVPBuffer, 64);
            constexpr float invSize[2] = {1.0f / shadowMapWidth, 1.0f / shadowMapHeight};
            uploadPass.uploadBuffer(&invSize[0], 8, _shadowVPBuffer, 128);
        }

        // shadow rendering pass
        {
            auto shadowPass = renderGraph.addRenderPass("shadowMap");
            shadowPass.addColor(_shadowMapRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {1.0f})
                .addDepthStencil(_shadowMapDS, graph::LoadOp::CLEAR, graph::StoreOp::DONT_CARE, graph::LoadOp::DONT_CARE, graph::StoreOp::DONT_CARE, 1.0, 0);
            auto shadowQ = shadowPass.addQueue("shadowMap");
            shadowQ.setViewport(0, 0, shadowMapWidth, shadowMapHeight, 0.0f, 1.0f)
                .addCamera(_shadowCam.get())
                .addUniformBuffer(_shadowVPBuffer, "Mat");
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
            Vec4f lightPos{3.0, 3.0, 3.0, 1.0};
            uploadPass.uploadBuffer(&lightPos[0], 16, _light, 0);
            uploadPass.uploadBuffer(&color[0], 16, _light, 16);
        }

        // rendering
        {
            auto renderPass = renderGraph.addRenderPass("forward");

            renderPass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.2, 0.4, 0.4, 1.0})
                .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::DONT_CARE, graph::LoadOp::DONT_CARE, graph::StoreOp::DONT_CARE, 1.0, 0);
            auto queue = renderPass.addQueue("solidColor");

            auto width = _swapchain->width();
            auto height = _swapchain->height();
            queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
                .addCamera(_cam.get())
                .addUniformBuffer(_camBuffer, "Mat")
                .addUniformBuffer(_shadowVPBuffer, "ShadowView")
                .addSampledImage(_shadowMapRT, "shadowMap")
                .addSampler(_shadowSampler, "shadowSampler");
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
    const std::string _shadowMapDS = "shadowMapDS";
    const std::string _camBuffer = "camBuffer";
    const std::string _camPose = "camPose";
    const std::string _shadowVPBuffer = "shadowVP";
    const std::string _light = "light";
    const std::string _shadowSampler = "shadowSampler";

    const std::string _name = "ShadowMap";

    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseButtonEventTag> _mouseListener;
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
};
} // namespace raum::sample