#pragma once
#include "Material.h"
#include "common.h"
#include "Mesh.h"

namespace raum::scene {

// deprecated
class Model {
public:
    Model();
    std::shared_ptr<Model> createInstance() const;
    std::vector<MeshRendererPtr>& meshRenderers();
    const std::vector<MeshRendererPtr>& meshRenderers() const;
    AABB& aabb();
    const AABB& aabb() const;

private:
    std::vector<MeshRendererPtr> _meshRenderers;
    AABB _aabb{};
};
using ModelPtr = std::shared_ptr<Model>;

struct Plane {
    Vec3f point;
    Vec3f normal;
};

using FrustumPlanes = std::array<Plane, 6>;

} // namespace raum::scene
