#include "Model.h"
#include "Mesh.h"

namespace raum::scene {

Model::Model() {
}

std::shared_ptr<Model> Model::createInstance() const {
    auto instance = std::make_shared<Model>();
    instance->_aabb = _aabb;
    instance->_meshRenderers.reserve(_meshRenderers.size());
    for (const auto& meshRenderer : _meshRenderers) {
        instance->_meshRenderers.emplace_back(meshRenderer->createInstance());
    }
    return instance;
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
