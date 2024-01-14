#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "GraphTypes.h"

namespace raum::graph {
struct Pass {
    std::string name{};
    std::variant<RenderPassData, SubRenderPassData, ComputePassData, CopyPassData> pass;
};
} // namespace raum::graph

namespace boost {
namespace graph {

/// Use the City name as a key for indexing cities in a graph
template <>
struct internal_vertex_name<raum::graph::Pass> {
    typedef multi_index::member<raum::graph::Pass, std::string, &raum::graph::Pass::name> type;
};

/// Allow the graph to build cities given only their names (filling in
/// the defaults for fields).
template <>
struct internal_vertex_constructor<raum::graph::Pass> {
    typedef vertex_from_name<raum::graph::Pass> type;
};

} // namespace graph
} // namespace boost

namespace raum::graph {

using RenderGraphImpl = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Pass, boost::no_property>;

class RenderPass {
public:
    RenderPass(RenderPassData& data) : _data(data) {}
    RenderPass(const RenderPass& rhs) : _data(rhs._data) {}
    RenderPass& operator=(const RenderPass& rhs) { _data = rhs._data; }

    RenderPass(RenderPass&& rhs) = delete;
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
    ComputePass(const ComputePass& rhs) : _data(rhs._data) {}
    ComputePass& operator=(const ComputePass& rhs) { _data = rhs._data; }

    ComputePass(ComputePass&& rhs) = delete;
    ~ComputePass() = default;

    ComputePass& addResource(std::string_view name, std::string_view bindingName, Access access);

private:
    ComputePassData& _data;
};

class CopyPass {
public:
    CopyPass(CopyPassData& data) : _data(data) {}
    CopyPass(const CopyPass& rhs) : _data(rhs._data) {}
    CopyPass& operator=(const CopyPass& rhs) { _data = rhs._data; }

    CopyPass(CopyPass&& rhs) = delete;
    ~CopyPass() = default;

    CopyPass& addPair(const CopyPair&);

private:
    CopyPassData& _data;
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
    CopyPass addCopyPass(std::string_view name);

private:
    RenderGraphImpl _graph;
};

} // namespace raum::graph