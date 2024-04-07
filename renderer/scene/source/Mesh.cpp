#include "Mesh.h"
#include "Model.h"

namespace raum::scene {

Mesh::Mesh(const MeshData& data, const Model& model)
:_data(data), _model(model) {
}

MeshData& Mesh::meshData() {
    return _data;
}

void Mesh::addTechnique(raum::scene::TechniquePtr tech) {
    _techs.emplace_back(tech);
}

void Mesh::removeTechnique(uint32_t index) {
    _techs.erase(_techs.begin() + index);
}

const MeshData& Mesh::meshData() const {
    return _data;
}

const Model& Mesh::model() const {
    return _model;
}

} // namespace raum::scene