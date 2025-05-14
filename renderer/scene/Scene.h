#pragma once
#include <stdint.h>
#include <vector>
#include "RHIDefine.h"
#include "Model.h"
#include <span>

namespace raum::scene {

class Scene {
public:
    Scene() {};

    std::vector<Model> models;
};

struct BVHNode {
    BVHNode* left{nullptr};
    BVHNode* right{nullptr};
    AABB aabb{};
    std::span<RenderablePtr> children;
};

void buildBVH(BVHNode* node, std::vector<scene::ModelPtr>& models);

// should be rendered / inside or intersect with frustum
bool frustumCulling(const scene::FrustumPlanes& frustum, const AABB& aabb);

float distance(const Vec3f& point, const Plane& plane);

} // namespace raum::scene