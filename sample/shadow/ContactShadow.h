
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
#include "sample/common.h"
#include "core/utils/utils.h"
#include "math.h"

namespace raum::sample {
class ContactShadowSample : public SampleBase {
    constexpr static uint32_t shadowMapWidth = 1024;
    constexpr static uint32_t shadowMapHeight = 1024;
    constexpr static Vec3f lightColor{1.0, 1.0, 1.0};

public:
    explicit ContactShadowSample(framework::Director* director) : _ppl(director->pipeline()), _director(director) {}

    void init() override;

    ~ContactShadowSample();

    void show() override;

    void hide() override;

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
    // const std::string _

    const std::string _name = "ContactShadowSample";

    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseButtonEventTag> _mouseListener;
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
};
} // namespace raum::sample