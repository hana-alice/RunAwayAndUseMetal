#include "RenderGraph.h"
#include "RHIImage.h"
#include "RHIBuffer.h"
#include "RHIImageView.h"
#include "RHIBufferView.h"

using boost::add_vertex;
using boost::get;
using boost::vertices;

namespace raum::graph {

RenderPass RenderGraph::addRenderPass(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    return RenderPass{std::get<RenderPassData>(_graph[id].pass)};
}

ComputePass RenderGraph::addComputePass(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    return ComputePass{std::get<ComputePassData>(_graph[id].pass)};
}

CopyPass RenderGraph::addCopyPass(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    return CopyPass{std::get<CopyPassData>(_graph[id].pass)};
}

RenderPass& RenderPass::addColor(std::string_view name) {
    _data.resources.emplace_back(std::string{name}, "", Access::WRITE, ResourceType::COLOR);
    return *this;
}

RenderPass& RenderPass::addDepthStencil(std::string_view name) {
    _data.resources.emplace_back(std::string{name}, "", Access::WRITE, ResourceType::DEPTH_STENCIL);
    return *this;
}

RenderPass& RenderPass::addShadingRate(std::string_view name) {
    _data.resources.emplace_back(std::string{name}, "", Access::WRITE, ResourceType::SHADING_RATE);
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
