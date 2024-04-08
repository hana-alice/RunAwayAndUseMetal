#include "Model.h"
#include "Mesh.h"

namespace raum::scene {

Model::Model() {
}


std::vector<MeshRendererPtr>& Model::meshRenderers() {
    return _meshRenderers;
}

const std::vector<MeshRendererPtr>& Model::meshRenderers() const {
    return _meshRenderers;
}

AABB& Model::aabb() {
    return _aabb;
}

const AABB& Model::aabb() const {
    return _aabb;
}

}