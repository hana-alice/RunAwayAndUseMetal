#include "common.h"
#include "Graphs.h"
#include "core/utils/utils.h"
#include "Serialization.h"
#include "Camera.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "PBRMaterial.h"
#include "RHIDevice.h"
#include "SceneLoader.h"
namespace raum::sample {

class AnimationModel: public SampleBase{
public:
    AnimationModel(rhi::DevicePtr device, rhi::SwapchainPtr swapchain, graph::GraphSchedulerPtr graph)
    :_device(device), _swapchain(swapchain), _graph(graph){
    }

    void init() override {
        auto& shaderGraph = _graph->shaderGraph();
        const auto& resourcePath = utils::resourceDirectory();
        graph::deserialize(resourcePath / "shader", "cook-torrance", shaderGraph);
        shaderGraph.compile("asset");

        asset::SceneLoader loader(_device);
        loader.loadFlat(resourcePath / "models" / "fantasy_compass" / "scene.gltf");
        auto model = loader.modelData();
        for (auto& meshRenderer : model->meshRenderers()) {
            auto pbrMat = std::static_pointer_cast<raum::scene::PBRMaterial>(meshRenderer->technique(0)->material());
            pbrMat->setDiffuse("mainTexture");
        }
        auto& sceneGraph = _graph->sceneGraph();
        auto& modelNode = sceneGraph.addModel("compass", "");
        modelNode.model = model;

        const auto& aabb = model->aabb();
        auto far = std::abs(aabb.maxBound.z);

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        scene::Frustum frustum{45.0f, width / (float)height, 0.01, 10 * far};
        _cam = std::make_shared<scene::Camera>(frustum, scene::Projection::PERSPECTIVE);
        auto& eye = _cam->eye();
        eye.setPosition(0.0f, 0.0f, 1.0);
        eye.lookAt({0.0, 0.0, -1.0}, {0.0f, 1.0f, 0.0f});
        eye.update();

        auto& resourceGraph = _graph->resourceGraph();
        if (!resourceGraph.contains(_forwardRT)) {
            //        _resourceGraph->addImage(_forwardRT, rhi::ImageUsage::COLOR_ATTACHMENT, width, height, rhi::Format::BGRA8_UNORM);
            resourceGraph.import(_forwardRT, _swapchain);
        }
        if (!resourceGraph.contains(_forwardDS)) {
            resourceGraph.addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, width, height, rhi::Format::D24_UNORM_S8_UINT);
        }
        if (!resourceGraph.contains(_camBuffer)) {
            resourceGraph.addBuffer(_camBuffer, 192, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
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
    void show() override {
        auto& renderGraph = _graph->renderGraph();
        auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");

        _graph->resourceGraph().updateImage("forwardDS", _swapchain->width(), _swapchain->height());

        auto& eye = _cam->eye();
        //    eye.translate(1.0, 0.0,  0.0);
        auto modelMat = Mat4(1.0);
        uploadPass.uploadBuffer(&modelMat[0], 64, _camBuffer, 0);
        auto viewMat = eye.inverseAttitide();
        uploadPass.uploadBuffer(&viewMat[0], 64, _camBuffer, 64);
        const auto& projMat = eye.projection();
        uploadPass.uploadBuffer(&projMat[0], 64, _camBuffer, 128);

        uploadPass.uploadBuffer(&eye.getPosition()[0], 12, _camPose, 0);
        Vec4f color{1.0, 1.0, 1.0, 1.0};
        Vec4f lightPos{10.0, 100.0, 0.0, 1.0};
        uploadPass.uploadBuffer(&lightPos[0], 16, _light, 0);
        uploadPass.uploadBuffer(&color[0], 16, _light, 16);

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

        _graph->execute();
    }

    void hide() override {
        _graph->sceneGraph().disable("compass");
    }

    const std::string& name() {
        return _name;
    }

private:
    const std::string _name{"AnimationModel"};

    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;
    graph::GraphSchedulerPtr _graph;
    scene::CameraPtr _cam;

    std::string _forwardRT{"forwardRT"};
    std::string _forwardDS{"forwardDS"};
    const std::string _camBuffer = "camBuffer";
    const std::string _camPose = "camPose";
    const std::string _light = "light";


    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseEventTag> _mouseListener;
};

}
