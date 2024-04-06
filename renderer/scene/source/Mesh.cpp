#include "Mesh.h"
#include "Model.h"

namespace raum::scene {

Mesh::Mesh(const MeshData& data, const Model& model)
:_data(data), _model(model) {
}

MeshData& Mesh::meshData() {
    return _data;
}

void Mesh::setMaterial(raum::scene::MaterialPtr material) {
    _material = material;
}

MaterialPtr Mesh::material() const {
    return _material;
}

const MeshData& Mesh::meshData() const {
    return _data;
}

const Model& Mesh::model() const {
    return _model;
}

} // namespace raum::scene