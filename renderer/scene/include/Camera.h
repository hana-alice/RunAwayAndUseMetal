#pragma once
#include <array>
#include "Eye.h"
namespace raum::scene {

constexpr uint32_t EyePerCam{1};

class Camera {
public:
    Camera() = delete;
    explicit Camera(const Frustum& frustum, Projection type);

    Eye& eye() { return _eye; }

private:
    Eye _eye;
};

}