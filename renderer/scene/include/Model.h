#pragma once
#include "Material.h"
#include "common.h"
#include "Phase.h"
#include "Mesh.h"

namespace raum::scene {

class Model {
public:
    Model();
    std::vector<MeshPtr>& meshes();
    const std::vector<MeshPtr>& meshes() const;
    std::vector<MaterialPtr >& materials();
    const std::vector<MaterialPtr>& materials() const;
    AABB& aabb();
    const AABB& aabb() const;

private:
    std::vector<MeshPtr> _meshes;
    std::vector<MaterialPtr> _materials;
    AABB _aabb{};
};
using ModelPtr = std::shared_ptr<Model>;

inline ModelPtr makeModel() {
    return std::make_shared<Model>();
}

} // namespace raum::scene