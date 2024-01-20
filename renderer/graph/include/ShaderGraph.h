#pragma once
#include <filesystem>
#include "GraphTypes.h"
#include <boost/graph/adjacency_list.hpp>

namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSetLayout;
}

namespace raum::graph {
struct ShaderResourceNode {
    std::string name{};
};
}

namespace boost {
namespace graph {

template <>
struct internal_vertex_name<raum::graph::ShaderResourceNode> {
    typedef multi_index::member<raum::graph::ShaderResourceNode, std::string, &raum::graph::ShaderResourceNode::name> type;
};

template <>
struct internal_vertex_constructor<raum::graph::ShaderResourceNode> {
    typedef vertex_from_name<raum::graph::ShaderResourceNode> type;
};

} // namespace graph
} // namespace boost

namespace raum::graph {

using ShaderGraphImpl = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, ShaderResourceNode, boost::no_property>;

class ShaderGraph {
public:
    ShaderGraph(rhi::RHIDevice* device);

    void load(const std::filesystem::path &path, std::string_view name);

    rhi::RHIDescriptorSetLayout getLayout(std::string_view name);

    const ShaderResource& layout(std::string_view name);

    ShaderGraphImpl& underlyingGraph() { return _impl; }

private:
    rhi::RHIDevice* _device{nullptr};
    std::unordered_map<std::string, ShaderResource, hash_string, std::equal_to<>> _resources;
    ShaderGraphImpl _impl;
};
}