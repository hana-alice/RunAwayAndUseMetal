#include "Model.h"
#include "Mesh.h"

namespace raum::scene {

Model::Model() {
}


std::vector<MeshPtr>& Model::meshes() {
    return _meshes;
}

const std::vector<MeshPtr>& Model::meshes() const {
    return _meshes;
}

AABB& Model::aabb() {
    return _aabb;
}

const AABB& Model::aabb() const {
    return _aabb;
}

std::vector<MaterialPtr>& Model::materials() {
    return _materials;
}

const std::vector<MaterialPtr>& Model::materials() const {
    return _materials;
}

}