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
    ShaderGraph(rhi::DevicePtr device);

    // compile shader and generate descriptorset layout, name can be a parent node which typed by directory,
    // in this case all children of 'name' will be compiled. [grouped multithreading]
    void compile(std::string_view name);

//    rhi::DescriptorSetLayoutInfo layoutInfo(std::string_view name, Rate rate);

    const ShaderResource& layout(std::string_view name) const;

    void addCustomLayout(ShaderResource&& layout, std::string_view name);

    void addVertex(const std::filesystem::path &logicPath, ShaderResource&& shaderResource);
private:
    rhi::DevicePtr _device;
    ShaderResources _resources;
    ShaderGraphImpl _impl;
};

using ShaderGraphPtr = std::shared_ptr<ShaderGraph>;
}