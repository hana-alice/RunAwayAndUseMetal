#pragma once
#include "BuiltinRes.h"
#include "Camera.h"
#include "Director.h"
#include "GraphScheduler.h"
#include "GraphUtils.h"
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
#include "RHIUtils.h"
namespace raum::sample {
class BistroSample : public SampleBase {
public:
    explicit BistroSample(framework::Director* director) : _ppl(director->pipeline()), _director(director) {}

    void init() override {
        // load scene from gltf
        _device = _director->device();
        _swapchain = _director->swapchain();

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        scene::PerspectiveFrustum frustum{60.0f, width / (float)height, 1.f, 1000.0};
        _cam = std::make_shared<scene::Camera>(frustum);

        const auto& resourcePath = utils::resourceDirectory();
        auto& sceneGraph = _director->sceneGraph();
        asset::serialize::load(sceneGraph, resourcePath / "models" / "sponza" / "sponza.gltf", _device);

        {
            scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/fullscreen/rasterBlit");
            auto quadMat = matTemplate->instantiate("asset/layout/fullscreen/rasterBlit", scene::MaterialType::CUSTOM);

            scene::Texture tex{
                .texture = rhi::defaultSampledImage(_device),
                .textureView = rhi::defaultSampledImageView(_device),
            };
            quadMat->set("mainTexture", tex);
            scene::Sampler sampler{
                rhi::SamplerInfo{
                    .magFilter = rhi::Filter::LINEAR,
                    .minFilter = rhi::Filter::LINEAR,
                },
            };
            quadMat->set("mainSampler", sampler);

            _rasterBlitTech = std::make_shared<scene::Technique>(quadMat, "default");
            _rasterBlitTech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
            auto& bs = _rasterBlitTech->blendInfo();
            bs.attachmentBlends.emplace_back();
        }

        auto& skybox = asset::BuiltinRes::skybox();
        graph::ModelNode& skyboxNode = sceneGraph.addModel("skybox");
        skyboxNode.model = skybox.model();
        skyboxNode.hint = graph::ModelHint::NO_CULLING;

        auto& eye = _cam->eye();
        eye.setPosition(0.0, 0.0f, 50.0f);
        eye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        _cam->update();

        graph::CameraNode& camNode = sceneGraph.addCamera("mainCamera");
        camNode.camera = _cam;

        auto& resourceGraph = _ppl->resourceGraph();
        if (!resourceGraph.contains(_presentBuffer)) {
            resourceGraph.import(_presentBuffer, _swapchain);
        }
        if (!resourceGraph.contains(_forwardDS)) {
            resourceGraph.addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, width, height, rhi::Format::D24_UNORM_S8_UINT);
            resourceGraph.addImage(_forwardRT, rhi::ImageUsage::COLOR_ATTACHMENT | rhi::ImageUsage::SAMPLED, width, height, rhi::Format::BGRA8_UNORM);
        }
        if (!resourceGraph.contains(_camBuffer)) {
            resourceGraph.addBuffer(_camBuffer, 128, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
            resourceGraph.addBuffer(_camPose, 12, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
            resourceGraph.addBuffer(_light, 32, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
        }

        // listeners
        auto keyHandler = [&]() {
            auto front = _cam->eye().forward();
            front = glm::normalize(front);
            auto right = glm::cross(_cam->eye().up(), front);
            right = glm::normalize(right);
            float sensitivity = 1.1f;
            if (framework::keyPressed(framework::Keyboard::W)) {
                _cam->eye().translate(front * Vec3f(sensitivity));
            }
            if (framework::keyPressed(framework::Keyboard::S)) {
                _cam->eye().translate(-front * Vec3f(sensitivity));
            }
            if (framework::keyPressed(framework::Keyboard::A)) {
                _cam->eye().translate(-right * Vec3f(sensitivity));
            }
            if (framework::keyPressed(framework::Keyboard::D)) {
                _cam->eye().translate(right * Vec3f(sensitivity));
            }
            _cam->update();
        };
        _keyListener.add(keyHandler);

        static bool firstPress{true};
        static bool pressed{false};
        static int32_t lastX = 0;
        static int32_t lastY = 0;

        auto mouseHandler = [&, width, height](float x, float y, framework::MouseButton btn, framework::ButtonStatus status) {
            if (status == framework::ButtonStatus::RELEASE) {
                firstPress = true;
                pressed = false;
            } else if (status == framework::ButtonStatus::PRESS && btn != framework::MouseButton::OTHER) {
                pressed = true;
            }
            if (pressed && firstPress) {
                firstPress = false;
                lastX = x;
                lastY = y;
            }
        };
        _mouseBtnListener.add(mouseHandler);

        auto mouseMovehandler = [&](float x, float y, float deltaXIn, float deltaYIn) {
            if (!pressed) return;
            static float curHDeg = 180.0f;
            static float curVDeg = 0.0f;
            auto deltaX = x - lastX;
            auto deltaY = y - lastY;
            lastX = x;
            lastY = y;
            curHDeg -= deltaX * 0.1f;
            curVDeg -= deltaY * 0.1f;
            auto curHRad = curHDeg / 180.0f * 3.141593f;
            auto curVRad = curVDeg / 180.0f * 3.141593f;

            deltaX = -deltaX / 180.0f * 3.141593f;
            deltaY = -deltaY / 180.0f * 3.141593f;

            Quaternion qx(Vec3f(curVRad, 0.0f, 0.0f));
            Quaternion qy(Vec3f(0.0f, curHRad, 0.0f));

            Vec3f xAxis{1.0f, 0.0f, 0.0f};
            Vec3f yAxis{0.0f, 1.0f, 0.0f};

            auto& eye = _cam->eye();
            eye.setOrientation(qy * qx);
            _cam->update();
        };
        _mouseMoveListener.add(mouseMovehandler);
    }

    ~BistroSample() {
        _keyListener.remove();
        _mouseBtnListener.remove();
        _mouseMoveListener.remove();
    }

    void show() override {
        auto& renderGraph = _ppl->renderGraph();
        auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");

        _ppl->resourceGraph().updateImage("forwardDS", _swapchain->width(), _swapchain->height());

        auto& eye = _cam->eye();
        auto p = eye.getPosition();
        auto r = eye.getOrientation();
        auto mm = glm::toMat4(r) * glm::translate(glm::mat4(1.0f), -p);

        auto viewMat = eye.attitude();

        uploadPass.uploadBuffer(&viewMat[0], 64, _camBuffer, 0);
        const auto& projMat = eye.projection();
        uploadPass.uploadBuffer(&projMat[0], 64, _camBuffer, 64);

        uploadPass.uploadBuffer(&eye.getPosition()[0], 12, _camPose, 0);
        Vec4f color{1.0, 1.0, 1.0, 1.0};
        Vec4f lightPos{5.0, 5.0, 0.0, 1.0};
        uploadPass.uploadBuffer(&lightPos[0], 16, _light, 0);
        uploadPass.uploadBuffer(&color[0], 16, _light, 16);

        auto basePass = renderGraph.addRenderPass("forward");
        basePass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.3, 0.3, 0.3, 1.0})
            .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::STORE, graph::LoadOp::CLEAR, graph::StoreOp::STORE, 1.0, 0);
        auto queue = basePass.addQueue("default");

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
            .addFlag(graph::RenderQueueFlags::GEOMETRY)
            .addCamera(_cam.get())
            .addUniformBuffer(_camBuffer, "Mat")
            .addUniformBuffer(_camPose, "CamPos")
            .addUniformBuffer(_light, "Light");

        auto rasterBlitPass = renderGraph.addRenderPass("quad");
        rasterBlitPass.addColor(_presentBuffer, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.3, 0.3, 0.3, 1.0});
        auto rasterBlitQ = rasterBlitPass.addQueue("default");
        rasterBlitQ.setViewport(0, 0, width, height, 0.0f, 1.0f)
            .setQuadTech(_rasterBlitTech)
            .addSampledImage(_forwardRT, "mainTexture");
    }

    void hide() override {
        _director->sceneGraph().disable("sponza");
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

    scene::TechniquePtr _rasterBlitTech;

    const std::string _presentBuffer = "presentBuffer";
    const std::string _forwardRT = "forwardRT";
    const std::string _forwardDS = "forwardDS";
    const std::string _camBuffer = "camBuffer";
    const std::string _camPose = "camPose";
    const std::string _light = "light";

    const std::string _name = "BistroSample";

    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseButtonEventTag> _mouseBtnListener;
    framework::EventListener<framework::MouseMotionEventTag> _mouseMoveListener;
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
};

} // namespace raum::sample