#include "Mesh.h"

namespace raum::scene {

MeshData& Mesh::meshData() {
    return _data;
}

const MeshData& Mesh::meshData() const {
    return _data;
}

MeshRenderer::MeshRenderer(MeshPtr mesh) : _mesh(mesh) {}

void MeshRenderer::addTechnique(TechniquePtr tech) {
    _techs.emplace_back(tech);
}

void MeshRenderer::removeTechnique(uint32_t index) {
    _techs.erase(_techs.begin() + index);
}

void MeshRenderer::setMesh(MeshPtr mesh) {
    _mesh = mesh;
}

void MeshRenderer::setVertexInfo(uint32_t firstVertex, uint32_t vertexCount, uint32_t indexCount) {
    _drawInfo.firstVertex = firstVertex;
    _drawInfo.vertexCount = vertexCount;
    _drawInfo.indexCount = indexCount;
}

void MeshRenderer::setInstanceInfo(uint32_t firstInstance, uint32_t instanceCount) {
    _drawInfo.firstInstance = firstInstance;
    _drawInfo.instanceCount = instanceCount;
}

TechniquePtr MeshRenderer::technique(uint32_t index) {
    return _techs[index];
}

TechniquePtr MeshRenderer::technique(std::string_view phase) {
    auto iter = std::find_if(_techs.begin(), _techs.end(), [phase](const TechniquePtr& tech) {
        return tech->phase()->phaseName() == phase;
    });
    return iter == _techs.end() ? nullptr : *iter;
}

const DrawInfo& MeshRenderer::drawInfo() const {
    return _drawInfo;
}


} // namespace raum::scene