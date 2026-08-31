#include "CameraSample.h"

#include <algorithm>
#include <utility>

#include "SceneGraph.h"

namespace raum::sample {

CameraSample::CameraSample(framework::Director* director, std::string name)
: SampleBase(director, std::move(name)) {}

std::optional<CameraControlState> CameraSample::cameraControlState() const {
    if (!_camera) {
        return std::nullopt;
    }
    return CameraControlState{
        .position = _camera->eye().getPosition(),
        .yawDegrees = _yawDegrees,
        .pitchDegrees = _pitchDegrees,
        .verticalFovDegrees = _camera->fov().value,
        .moveSpeed = _flyConfig.moveSpeed,
    };
}

void CameraSample::applyCameraControlState(const CameraControlState& state) {
    if (!_camera) {
        return;
    }
    _yawDegrees = state.yawDegrees;
    _pitchDegrees = std::clamp(state.pitchDegrees, _flyConfig.minimumPitch, _flyConfig.maximumPitch);
    _camera->eye().setPosition(state.position);
    _camera->setFov(utils::Degree{std::clamp(state.verticalFovDegrees, 1.0f, 179.0f)});
    _flyConfig.moveSpeed = std::clamp(state.moveSpeed, 0.05f, 100.0f);
    updateCameraOrientation();
}

std::optional<LightingControlState> CameraSample::lightingControlState() const {
    return _lighting;
}

void CameraSample::applyLightingControlState(const LightingControlState& state) {
    constexpr float MinimumDirectionLengthSquared = 0.000001f;
    const float directionLengthSquared = glm::dot(state.direction, state.direction);
    if (directionLengthSquared > MinimumDirectionLengthSquared) {
        _lighting.direction = state.direction * glm::inversesqrt(directionLengthSquared);
    }
    _lighting.color = glm::max(state.color, Vec3f{0.0f});
}

void CameraSample::createPerspectiveCamera(const PerspectiveCameraConfig& config) {
    const auto height = std::max(viewportHeight(), 1u);
    const float aspect = static_cast<float>(viewportWidth()) / static_cast<float>(height);
    const scene::PerspectiveFrustum frustum{
        config.verticalFov,
        aspect,
        config.nearPlane,
        config.farPlane,
    };
    _camera = std::make_shared<scene::Camera>(frustum);
    _camera->eye().setPosition(config.position);
    _yawDegrees = config.yawDegrees;
    _pitchDegrees = config.pitchDegrees;
    updateCameraOrientation();

    const auto& cameraNodeName = resource("cameraNode");
    sceneGraph().addCamera(cameraNodeName).camera = _camera;
    trackSceneNode(cameraNodeName);
}

void CameraSample::enableFlyCamera(const FlyCameraConfig& config) {
    _flyConfig = config;
    if (_flyCameraEnabled) {
        return;
    }
    _flyCameraEnabled = true;

    _mouseButtonListener.add([this](float, float, framework::MouseButton button, framework::ButtonStatus status) {
        if (!active()) {
            _dragging = false;
            return;
        }
        if (status == framework::ButtonStatus::RELEASE) {
            _dragging = false;
        } else if (button != framework::MouseButton::OTHER) {
            _dragging = true;
        }
    });

    _mouseMotionListener.add([this](float, float, float deltaX, float deltaY) {
        if (!active() || !_dragging || !_camera) {
            return;
        }
        _yawDegrees -= deltaX * _flyConfig.lookSensitivity;
        _pitchDegrees = std::clamp(
            _pitchDegrees - deltaY * _flyConfig.lookSensitivity,
            _flyConfig.minimumPitch,
            _flyConfig.maximumPitch);
        updateCameraOrientation();
    });
}

void CameraSample::onUpdate(std::chrono::milliseconds deltaTime) {
    if (!_flyCameraEnabled || !_camera) {
        return;
    }

    auto& eye = _camera->eye();
    const auto front = glm::normalize(eye.forward());
    // Raum uses a left-handed camera basis (+Z forward), so right is up x forward.
    const auto right = glm::normalize(glm::cross(eye.up(), front));
    Vec3f direction{0.0f};
    if (framework::keyPressed(framework::Keyboard::W)) {
        direction += front;
    }
    if (framework::keyPressed(framework::Keyboard::S)) {
        direction -= front;
    }
    if (framework::keyPressed(framework::Keyboard::A)) {
        direction -= right;
    }
    if (framework::keyPressed(framework::Keyboard::D)) {
        direction += right;
    }

    const auto lengthSquared = glm::dot(direction, direction);
    if (lengthSquared <= 0.0f) {
        return;
    }

    constexpr float MaxDeltaSeconds = 0.05f;
    const auto deltaSeconds = std::clamp(
        static_cast<float>(deltaTime.count()) / 1000.0f,
        0.0f,
        MaxDeltaSeconds);
    eye.translate(glm::normalize(direction) * _flyConfig.moveSpeed * deltaSeconds);
    _camera->update();
}

void CameraSample::ensureCameraResources(bool includePosition) {
    const auto usage = rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST;
    ensureBuffer("camera", 2 * sizeof(Mat4), usage);
    if (includePosition) {
        ensureBuffer("cameraPosition", sizeof(Vec3f), usage);
    }
}

void CameraSample::ensureLightResource() {
    ensureBuffer("light", 2 * sizeof(Vec4f), rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST);
}

void CameraSample::uploadCamera(graph::CopyPass& uploadPass, bool includePosition) {
    const auto& eye = _camera->eye();
    const auto& view = eye.attitude();
    uploadPass.uploadBuffer(&view[0], sizeof(Mat4), cameraBuffer(), 0);
    const auto& projection = eye.projection();
    uploadPass.uploadBuffer(&projection[0], sizeof(Mat4), cameraBuffer(), sizeof(Mat4));
    if (includePosition) {
        uploadPass.uploadBuffer(&eye.getPosition()[0], sizeof(Vec3f), cameraPositionBuffer(), 0);
    }
}

void CameraSample::uploadLight(graph::CopyPass& uploadPass) {
    const Vec4f direction{glm::normalize(_lighting.direction), 0.0f};
    const Vec4f color{_lighting.color, 1.0f};
    uploadPass.uploadBuffer(&direction[0], sizeof(Vec4f), lightBuffer(), 0);
    uploadPass.uploadBuffer(&color[0], sizeof(Vec4f), lightBuffer(), sizeof(Vec4f));
}

void CameraSample::updateCameraOrientation() {
    const Quaternion pitch(Vec3f(glm::radians(_pitchDegrees), 0.0f, 0.0f));
    const Quaternion yaw(Vec3f(0.0f, glm::radians(_yawDegrees), 0.0f));
    _camera->eye().setOrientation(yaw * pitch);
    _camera->update();
}

} // namespace raum::sample
