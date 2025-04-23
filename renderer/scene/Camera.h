#pragma once
#include <array>
#include <memory>
#include "Eye.h"

#include "Model.h"
namespace raum::scene {

constexpr uint32_t EyePerCam{1};

class Camera {
public:
    Camera() = delete;
    explicit Camera(const PerspectiveFrustum& frustum);
    explicit Camera(const OrthoFrustum& frustum);

    Eye& eye() { return _eye; }

    const Eye& eye() const { return _eye; }

    const FrustumPlanes& frustumPlanes() const { return _frustumPlanes; };

    void update();

    void enableCulling() { _culling = true; }
    void disableCulling() { _culling = false; }
    bool cullingEnabled() const { return _culling; }
private:
    Eye _eye;
    FrustumPlanes _frustumPlanes;
    bool _culling{true};
};

using CameraPtr = std::shared_ptr<Camera>;

}