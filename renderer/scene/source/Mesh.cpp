#include "Mesh.h"
#include "Model.h"

namespace raum::scene {

Mesh::Mesh(const MeshData& data)
:_data(data) {
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


} // namespace raum::scene