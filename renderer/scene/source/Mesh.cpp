#include "Mesh.h"
#include "RHIUtils.h"

namespace raum::scene {

Mesh::Mesh(raum::scene::MeshType type):_type(type) {
    if(_type == scene::MeshType::MESH) {
        _meshletsData = std::make_shared<MeshletData>();
    }
}

MeshData& Mesh::meshData() {
    return _data;
}

const MeshData& Mesh::meshData() const {
    return _data;
}

const MeshType Mesh::type() const {
    return _type;
}

MeshletData& Mesh::meshletData() {
    return *_meshletsData;
}

const MeshletData& Mesh::meshletData() const {
    return *_meshletsData;
}

MeshRenderer::MeshRenderer(MeshPtr mesh) : _mesh(mesh) {

}

void MeshRenderer::prepare(rhi::DevicePtr device) {
    if(_mesh->type() == scene::MeshType::MESH) {
        std::vector<rhi::RHISampler*> vec;
        rhi::DescriptorSetLayoutInfo info{};
        // meshlet count
        info.descriptorBindings.emplace_back(
            0,
            rhi::DescriptorType::UNIFORM_BUFFER,
            1,
            rhi::ShaderStage::TASK,
            vec
            );
        // meshlet
        info.descriptorBindings.emplace_back(
            1,
            rhi::DescriptorType::STORAGE_BUFFER,
            1,
            rhi::ShaderStage::TASK | rhi::ShaderStage::MESH,
            vec
        );
        // primitive indices
        info.descriptorBindings.emplace_back(
            2,
            rhi::DescriptorType::STORAGE_BUFFER,
            1,
            rhi::ShaderStage::MESH,
            vec
        );
        // vertex indices
        info.descriptorBindings.emplace_back(
            3,
            rhi::DescriptorType::STORAGE_BUFFER,
            1,
            rhi::ShaderStage::MESH,
            vec
        );
        // vertex buffer
        info.descriptorBindings.emplace_back(
            4,
            rhi::DescriptorType::STORAGE_BUFFER,
            1,
            rhi::ShaderStage::MESH,
            vec
        );
        auto layout = rhi::getOrCreateDescriptorSetLayout(info, device);

        boost::container::flat_map<std::string_view, uint32_t> bindings = {
            {"MeshletCount", 0},
            {"Meshlets", 1},
            {"PrimitiveIndices", 2},
            {"VertexIndices", 3},
            {"Vertices", 4},
        };

        _mesh->meshletData().bindGroup = std::make_shared<BindGroup>(bindings, layout, device);

        auto meshletData = _mesh->meshletData();
        auto bindGroup  = meshletData.bindGroup;
        bindGroup->bindBuffer("MeshletCount", 0, meshletData.meshletCountBuffer);
        bindGroup->bindBuffer("Meshlets", 0, meshletData.meshletsBuffer);
        bindGroup->bindBuffer("PrimitiveIndices", 0, meshletData.primIndicesBuffer);
        bindGroup->bindBuffer("VertexIndices", 0, meshletData.vertexIndicesBuffer);
        bindGroup->bindBuffer("Vertices", 0, meshletData.vertexBuffer);
        bindGroup->update();
    }
}

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

const MeshPtr& MeshRenderer::mesh() const {
    return _mesh;
}

TechniquePtr MeshRenderer::technique(uint32_t index) {
    return _techs[index];
}

std::vector<TechniquePtr>& MeshRenderer::techniques() {
    return _techs;
}

const DrawInfo& MeshRenderer::drawInfo() const {
    return _drawInfo;
}

} // namespace raum::scene