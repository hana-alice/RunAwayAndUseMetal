#pragma once

#include "Camera.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"
#include "RenderGraph.h"
#include "SampleBase.h"

namespace raum::sample {

struct PerspectiveCameraConfig {
    utils::Degree verticalFov{60.0f};
    float nearPlane{0.1f};
    float farPlane{1000.0f};
    Vec3f position{0.0f, 0.0f, 4.0f};
    float yawDegrees{180.0f};
    float pitchDegrees{0.0f};
};

struct FlyCameraConfig {
    // World units per second.
    float moveSpeed{4.0f};
    float lookSensitivity{0.1f};
    float minimumPitch{-89.9f};
    float maximumPitch{89.9f};
};

class CameraSample : public SampleBase {
public:
    CameraSample(framework::Director* director, std::string name);

    std::optional<CameraControlState> cameraControlState() const override;
    void applyCameraControlState(const CameraControlState& state) override;
    std::optional<LightingControlState> lightingControlState() const override;
    void applyLightingControlState(const LightingControlState& state) override;

protected:
    void createPerspectiveCamera(const PerspectiveCameraConfig& config = {});
    void enableFlyCamera(const FlyCameraConfig& config = {});

    bool hasCamera() const { return static_cast<bool>(_camera); }
    scene::Camera& camera() const { return *_camera; }
    const scene::CameraPtr& cameraPtr() const { return _camera; }

    void ensureCameraResources(bool includePosition = true);
    void ensureLightResource();
    void uploadCamera(graph::CopyPass& uploadPass, bool includePosition = true);
    void uploadLight(graph::CopyPass& uploadPass);

    const std::string& cameraBuffer() const { return resource("camera"); }
    const std::string& cameraPositionBuffer() const { return resource("cameraPosition"); }
    const std::string& lightBuffer() const { return resource("light"); }

private:
    void onUpdate(std::chrono::milliseconds deltaTime) override;
    void updateCameraOrientation();

    scene::CameraPtr _camera;
    FlyCameraConfig _flyConfig;
    float _yawDegrees{0.0f};
    float _pitchDegrees{0.0f};
    LightingControlState _lighting;
    bool _flyCameraEnabled{false};
    bool _dragging{false};
    framework::EventListener<framework::MouseButtonEventTag> _mouseButtonListener;
    framework::EventListener<framework::MouseMotionEventTag> _mouseMotionListener;
};

} // namespace raum::sample
