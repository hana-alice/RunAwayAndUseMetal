#pragma once
#include <stdint.h>
#include <vector>
#include "RHIDefine.h"
#include "Model.h"
namespace raum::scene {

class Scene {
public:
    Scene() {};

    std::vector<Model> models;
};

// should be rendered / inside or intersect with frustum
bool frustumCulling(const scene::FrustumPlanes& frustum, const AABB& aabb);

float distance(const Vec3f& point, const Plane& plane);

} // namespace raum::scene