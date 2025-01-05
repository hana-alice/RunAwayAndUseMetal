#pragma once
#include <array>
#include "Eye.h"
#include <memory>
namespace raum::scene {

constexpr uint32_t EyePerCam{1};

class Camera {
public:
    Camera() = delete;
    explicit Camera(const PerspectiveFrustum& frustum);
    explicit Camera(const OrthoFrustum& frustum);

//    void setPosition()

    Eye& eye() { return _eye; }

private:
    Eye _eye;
};

using CameraPtr = std::shared_ptr<Camera>;

}