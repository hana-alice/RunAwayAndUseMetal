#pragma once
#include "Camera.h"
#include "Graphs.h"
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
class GraphSample : public SampleBase {
public:
    explicit GraphSample(rhi::DevicePtr device, rhi::SwapchainPtr swapchain, graph::GraphSchedulerPtr graphScheduler)
    : _device(device), _swapchain(swapchain), _graphScheduler(graphScheduler) {}

    void init() override {
        auto& shaderGraph = _graphScheduler->shaderGraph();

        // deserialize layout(json): shader description
        const auto& resourcePath = utils::resourceDirectory();
        graph::deserialize(resourcePath / "shader", "cook-torrance", shaderGraph);

        // compile shaders
        shaderGraph.compile("asset");

        // load scene from gltf
        auto& sceneGraph = _graphScheduler->sceneGraph();
        asset::serialize::load(sceneGraph, resourcePath / "models" / "sponza" / "sponza.gltf", _device);

        // reflect shader slot for material and mesh local data
        std::ranges::for_each(sceneGraph.models(), [](auto& node) {
            auto& model = std::get<graph::ModelNode>(node.get().sceneNodeData);
            std::ranges::for_each(model.model->meshRenderers(), [](auto meshRenderer) {
                meshRenderer->setTransformSlot("LocalMat");
            });
        });

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        if (sceneGraph.cameras().empty()) {
            scene::Frustum frustum{45.0f, width / (float)height, 0.1f, 1000.0f};
            _cam = std::make_shared<scene::Camera>(frustum, scene::Projection::PERSPECTIVE);
            auto& eye = _cam->eye();
            eye.setPosition(0.0, 10.0, 0.0);
            eye.lookAt({100.0f, 10.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
            eye.update();
        } else {
            // :))))))
            const auto& camNode = sceneGraph.cameras().front().get();
            _cam = std::get<graph::CameraNode>(camNode.sceneNodeData).camera;
            auto& eye = _cam->eye();
            eye.setTransform(camNode.node.transform());
            eye.update();
        }



        auto& resourceGraph = _graphScheduler->resourceGraph();
        if (!resourceGraph.contains(_forwardRT)) {
            resourceGraph.import(_forwardRT, _swapchain);
        }
        if (!resourceGraph.contains(_forwardDS)) {
            resourceGraph.addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, width, height, rhi::Format::D24_UNORM_S8_UINT);
        }
        if (!resourceGraph.contains(_camBuffer)) {
            resourceGraph.addBuffer(_camBuffer, 128, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
            resourceGraph.addBuffer(_camPose, 12, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
            resourceGraph.addBuffer(_light, 32, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
        }

        // listeners
        auto keyHandler = [&](framework::Keyboard key, framework::KeyboardType type) {
            if (key != framework::Keyboard::OTHER && type == framework::KeyboardType::PRESS) {
                auto front = _cam->eye().forward();
                front = glm::normalize(front);
                auto right = glm::cross(front, _cam->eye().up());
                right = glm::normalize(right);
                if (key == framework::Keyboard::W) {
                    _cam->eye().translate(front);
                } else if (key == framework::Keyboard::S) {
                    _cam->eye().translate(-front);
                } else if (key == framework::Keyboard::A) {
                    _cam->eye().translate(-right);
                } else if (key == framework::Keyboard::D) {
                    _cam->eye().translate(right);
                }
                _cam->eye().update();
            }
        };
        _keyListener.add(keyHandler);

        auto mouseHandler = [&, width, height](int32_t x, int32_t y, framework::MouseButton btn, framework::ButtonStatus status) {
            static int32_t lastX = x;
            static int32_t lastY = y;

            auto deltaX = x - lastX;
            auto deltaY = y - lastY;

            float factorX = deltaX / (float)width;
            float factorY = deltaY / (float)height;

            auto& eye = _cam->eye();
            auto right = glm::cross(eye.forward(), eye.up());

            _cam->eye().rotate(right, scene::Degree{-90.0f * factorY});
            _cam->eye().rotate(eye.up(), scene::Degree{-90.0f * factorX});

            _cam->eye().update();

            lastX = x;
            lastY = y;
        };
        _mouseListener.add(mouseHandler);
    }

    ~GraphSample() {
        _keyListener.remove();
        _mouseListener.remove();
    }

    void show() override {
        auto& renderGraph = _graphScheduler->renderGraph();
        auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");

        _graphScheduler->resourceGraph().updateImage("forwardDS", _swapchain->width(), _swapchain->height());

        auto& eye = _cam->eye();
        auto viewMat = eye.inverseAttitide();
        uploadPass.uploadBuffer(&viewMat[0], 64, _camBuffer, 0);
        const auto& projMat = eye.projection();
        uploadPass.uploadBuffer(&projMat[0], 64, _camBuffer, 64);

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

    void hide() override {
        _graphScheduler->sceneGraph().disable("sponza");
    }

    const std::string& name() override {
        return _name;
    }

private:
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;

    graph::GraphSchedulerPtr _graphScheduler;

    std::shared_ptr<scene::Camera> _cam;
    std::shared_ptr<scene::Scene> _scene;

    const std::string _forwardRT = "forwardRT";
    const std::string _forwardDS = "forwardDS";
    const std::string _camBuffer = "camBuffer";
    const std::string _camPose = "camPose";
    const std::string _light = "light";

    const std::string _name = "GraphSample";

    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseEventTag> _mouseListener;
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
};

} // namespace raum::sample