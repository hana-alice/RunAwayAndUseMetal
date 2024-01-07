#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "GraphTypes.h"
namespace raum::graph {

using Pass = std::variant<RenderPassData, SubRenderPassData, ComputePassData, CopyPassData>;

using RenderGraphImpl = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Pass, boost::property<boost::edge_weight_t, double>>;

class RenderPass {
public:
    RenderPass(RenderPassData& data) : _data(data) {}
    RenderPass(RenderPass&& rhs) : _data(rhs._data){};

    RenderPass(const RenderPass&) = delete;
    RenderPass& operator=(const RenderPass&) = delete;
    ~RenderPass() = default;

    RenderPass& addColor(std::string_view name);
    RenderPass& addDepthStencil(std::string_view name);
    RenderPass& addShadingRate(std::string_view name);

private:
    RenderPassData& _data;
};

class ComputePass {
public:
    ComputePass(ComputePassData& data) : _data(data) {}
    ComputePass(ComputePass&& rhs) : _data(rhs._data) {}

    ComputePass(const ComputePass&) = delete;
    ComputePass& operator=(const ComputePass&) = delete;
    ~ComputePass() = default;

    ComputePass& addResource(std::string_view name, std::string_view bindingName, Access access);

private:
    ComputePassData& _data;
};

class RenderGraph {
public:
    RenderGraph() = default;
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = delete;

    ~RenderGraph() = default;

    RenderPass addRenderPass(std::string_view name);
    ComputePass addComputePass(std::string_view name);

private:
    RenderGraphImpl _impl;
};

} // namespace raum::graph