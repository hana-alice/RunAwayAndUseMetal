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

RenderPass& RenderPass::addColor(std::string_view name) {
    auto& data = std::get<RenderPassData>(_graph[_id].data);
    data.attachments.emplace_back(std::string{name}, "", Access::WRITE, ResourceType::COLOR);
    return *this;
}

RenderPass& RenderPass::addDepthStencil(std::string_view name) {
    auto& data = std::get<RenderPassData>(_graph[_id].data);
    data.attachments.emplace_back(std::string{name}, "", Access::WRITE, ResourceType::DEPTH_STENCIL);
    return *this;
}

RenderPass& RenderPass::addShadingRate(std::string_view name) {
    auto& data = std::get<RenderPassData>(_graph[_id].data);
    data.attachments.emplace_back(std::string{name}, "", Access::WRITE, ResourceType::SHADING_RATE);
    return *this;
}

RenderQueue RenderPass::addQueue(std::string_view name) {
    auto outs = out_degree(_id, _graph);
    auto id = add_vertex(_graph[_id].name.append("_").append(name), _graph);
    _graph[id].data = RenderQueueData{};
    return RenderQueue{id, _graph};
}

// RenderQueue& RenderQueue::addQuad() {
//     auto& data = std::get<RenderQueueData>(_graph[_id].data);
//
//     return *this;
// }

RenderQueue& RenderQueue::addScene(scene::Scene* scene) {
    auto& data = std::get<RenderQueueData>(_graph[_id].data);
    data.scene = scene;
    return *this;
}

RenderQueue& RenderQueue::addCamera(scene::Camera* camera) {
    auto& data = std::get<RenderQueueData>(_graph[_id].data);
    data.camera = camera;
    return *this;
}

RenderQueue& RenderQueue::setViewport(int32_t x, int32_t y, uint32_t w, uint32_t h, float minDepth, float maxDepth) {
    auto& data = std::get<RenderQueueDatacd vck>(_graph[_id].data);
    data.viewport = {{x, y, w, h}, minDepth, maxDepth};
    return *this;
}

ComputePass& ComputePass::addResource(std::string_view name, std::string_view bindingname, Access access) {
    _data.resources.emplace_back(std::string{name}, std::string{bindingname}, access);
    return *this;
}

CopyPass& CopyPass::addPair(const CopyPair& pair) {
    _data.pairs.emplace_back(pair);
    return *this;
}

} // namespace raum::graph
