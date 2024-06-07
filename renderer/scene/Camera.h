#pragma once
#include <array>
#include "Eye.h"
#include <memory>
namespace raum::scene {

constexpr uint32_t EyePerCam{1};

class Camera {
public:
    Camera() = delete;
    explicit Camera(const Frustum& frustum, Projection type);

//    void setPosition()

    Eye& eye() { return _eye; }

private:
    Eye _eye;
};

using CameraPtr = std::shared_ptr<Camera>;

}