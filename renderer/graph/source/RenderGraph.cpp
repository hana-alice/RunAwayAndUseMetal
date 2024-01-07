#include "RenderGraph.h"
using boost::add_vertex;
using boost::get;
using boost::vertices;
namespace raum::graph {

RenderPass RenderGraph::addRenderPass(std::string_view name) {
    RenderPassData pass{std::string{name}};
    auto id = add_vertex(pass, _impl);
    return RenderPass{std::get<RenderPassData>(_impl[id])};
}

ComputePass RenderGraph::addComputePass(std::string_view name) {
    ComputePassData pass{std::string(name)};
    auto id = add_vertex(pass, _impl);
    return ComputePass{std::get<ComputePassData>(_impl[id])};
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

} // namespace raum::graph
