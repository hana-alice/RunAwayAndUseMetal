#pragma once
#include "Material.h"
#include "common.h"
#include "Phase.h"

namespace raum::scene {
class Mesh;
using MeshPtr = std::shared_ptr<Mesh>;

class Model {
public:
    Model();
    std::vector<MeshPtr>& meshes();
    const std::vector<MeshPtr>& meshes() const;
    std::vector<MaterialPtr >& materials();
    const std::vector<MaterialPtr>& materials() const;
    AABB& aabb();
    const AABB& aabb() const;

    void addPhase(PhasePtr phase);
    void removePhase(std::string_view phaseName);

private:
    std::vector<MeshPtr> _meshes;
    std::vector<MaterialPtr> _materials;
    AABB _aabb{};
    std::vector<PhasePtr> _phases;
};
using ModelPtr = std::shared_ptr<Model>;

inline ModelPtr makeModel() {
    return std::make_shared<Model>();
}

} // namespace raum::scene