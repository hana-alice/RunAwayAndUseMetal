#include "RenderGraph.h"
#include "RHIBuffer.h"
#include "RHIBufferView.h"
#include "RHIImage.h"
#include "RHIImageView.h"

using boost::add_vertex;
using boost::get;
using boost::out_degree;
using boost::vertex;
using boost::vertices;
using boost::add_edge;

namespace raum::graph {
RenderPass RenderGraph::addRenderPass(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].data = RenderPassData{};
    return RenderPass{id, _graph};
}

ComputePass RenderGraph::addComputePass(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].data = ComputePassData{};
    return ComputePass{std::get<ComputePassData>(_graph[id].data)};
}

CopyPass RenderGraph::addCopyPass(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].data = CopyPassData{};
    return CopyPass{std::get<CopyPassData>(_graph[id].data)};
}

void RenderGraph::clear() {
    _graph.clear();
}

RenderPass& RenderPass::addColor(std::string_view name, LoadOp loadOp, StoreOp storeOp, const ClearValue& color) {
    auto& data = std::get<RenderPassData>(_graph[_id].data);
    data.attachments.emplace_back(std::string{name}, "", color, Access::WRITE, ResourceType::COLOR, loadOp, storeOp);
    return *this;
}

RenderPass& RenderPass::addDepthStencil(std::string_view name, LoadOp loadOp, StoreOp storeOp, LoadOp stencilLoad, StoreOp stencilStore, float clearDepth, uint32_t clearStencil) {
    auto& data = std::get<RenderPassData>(_graph[_id].data);
    ClearValue ds{.depthStencil = {clearDepth, clearStencil}};
    data.attachments.emplace_back(std::string{name}, "", ds, Access::WRITE, ResourceType::DEPTH_STENCIL, loadOp, storeOp, stencilLoad, stencilStore);
    return *this;
}

RenderPass& RenderPass::addShadingRate(std::string_view name) {
    auto& data = std::get<RenderPassData>(_graph[_id].data);
    data.attachments.emplace_back(std::string{name}, "", ClearValue{}, Access::WRITE, ResourceType::SHADING_RATE);
    return *this;
}

RenderQueue RenderPass::addQueue(std::string_view name) {
    auto outs = out_degree(_id, _graph);
    auto id = add_vertex(name.data(), _graph);
    _graph[id].data = RenderQueueData{};
    add_edge(_id, id, _graph);
    return RenderQueue{id, _graph};
}

// RenderQueue& RenderQueue::addQuad() {
//     auto& data = std::get<RenderQueueData>(_graph[_id].data);
//
//     return *this;
// }

//RenderQueue& RenderQueue::addScene(scene::Scene* scene) {
//    auto& data = std::get<RenderQueueData>(_graph[_id].data);
//    data.scene = scene;
//    return *this;
//}

RenderQueue& RenderQueue::addCamera(scene::Camera* camera) {
    auto& data = std::get<RenderQueueData>(_graph[_id].data);
    data.camera = camera;
    return *this;
}

RenderQueue& RenderQueue::addUniformBuffer(std::string_view name, std::string_view bindingName) {
    auto& data = std::get<RenderQueueData>(_graph[_id].data);
    auto& resource = data.resources.emplace_back();
    resource.name = name;
    resource.bindingName = bindingName;
    resource.access = Access::READ;
    return *this;
}

RenderQueue& RenderQueue::setViewport(int32_t x, int32_t y, uint32_t w, uint32_t h, float minDepth, float maxDepth) {
    auto& data = std::get<RenderQueueData>(_graph[_id].data);
    data.viewport = {{x, y, w, h}, minDepth, maxDepth};
    return *this;
}

ComputePass& ComputePass::addResource(std::string_view name, std::string_view bindingname, Access access) {
    _data.resources.emplace_back(std::string{name}, std::string{bindingname}, access);
    return *this;
}

CopyPass& CopyPass::addPair(const CopyPair& pair) {
    _data.copies.emplace_back(pair);
    return *this;
}

CopyPass& CopyPass::uploadBuffer(const void* const data, uint32_t size, std::string_view name, uint32_t dstOffset) {
    std::vector<uint8_t> dataIn(size);
    memcpy(dataIn.data(), data, size);
    _data.uploads.emplace_back(dataIn, size, dstOffset, name.data());
    return *this;
}

} // namespace raum::graph
