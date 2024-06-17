#include "Mesh.h"
#include "RHICommandBuffer.h"
#include "RHIBlitEncoder.h"
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

const MeshPtr& MeshRenderer::mesh() const {
    return _mesh;
}

TechniquePtr MeshRenderer::technique(uint32_t index) {
    return _techs[index];
}

std::vector<TechniquePtr>& MeshRenderer::techniques() {
    return _techs;
}

BindGroupPtr MeshRenderer::bindGroup() {
    return _bindGroup;
}

const DrawInfo& MeshRenderer::drawInfo() const {
    return _drawInfo;
}

void MeshRenderer::setTransform(const Mat4& transform) {
    _transform = transform;
    _dirty = true;
}

void MeshRenderer::setTransformSlot(std::string_view name) {
    _localSLotName = name;
}

void MeshRenderer::prepare(
    const boost::container::flat_map<std::string_view, uint32_t>& bindings,
    rhi::DescriptorSetLayoutPtr layout,
    rhi::DevicePtr device) {
    if(!_bindGroup) {
        _bindGroup = std::make_shared<BindGroup>(bindings, layout, device);
        rhi::BufferSourceInfo bsInfo{
            .bufferUsage = rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST,
            .size = 16 * sizeof(float),
            .data = &_transform[0],
        };
        _localBuffer = rhi::BufferPtr(device->createBuffer(bsInfo));
    }
}

void MeshRenderer::update(rhi::CommandBufferPtr cmdBuffer) {
    if (!_localSLotName.empty()) {
        if (_dirty) {
            auto blitEncoder = rhi::BlitEncoderPtr(cmdBuffer->makeBlitEncoder());
            blitEncoder->updateBuffer(_localBuffer.get(), 0, &_transform[0], 16 * sizeof(float));
        }
        _bindGroup->bindBuffer(_localSLotName, 0, _localBuffer);
        _bindGroup->update();
    }
}


} // namespace raum::scene