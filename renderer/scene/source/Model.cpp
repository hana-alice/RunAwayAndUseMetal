#include "Model.h"
#include "Mesh.h"

namespace raum::scene {

Model::Model() {
    auto phase = getOrCreatePhase("simple-opaque");
    phase->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_STRIP);
    rhi::RasterizationInfo rasterizationInfo{};
    phase->setRasterizationInfo(rasterizationInfo);
    rhi::DepthStencilInfo depthStencilInfo{};
    depthStencilInfo.depthTestEnable = true;
    depthStencilInfo.depthWriteEnable = true;
    phase->setDepthStencilInfo(depthStencilInfo);

    _phases.emplace_back(phase);
}

void Model::addPhase(raum::scene::PhasePtr phase) {
    _phases.emplace_back(phase);
}

void Model::removePhase(std::string_view phaseName) {
    auto phase = getOrCreatePhase(phaseName);
    std::erase_if(_phases, [&phase](PhasePtr ele){
        return ele.get() == phase.get();
    });
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