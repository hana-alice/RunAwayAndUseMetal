#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "GraphTypes.h"

namespace raum::rhi {
class RHIDevice;
}

namespace raum::graph {

struct Resource {
    std::string name{};
    uint64_t life{0};
    std::variant<BufferData, BufferViewData, ImageData, ImageViewData> resource;
};
} // namespace raum::graph

namespace boost {
namespace graph {

/// Use the City name as a key for indexing cities in a graph
template <>
struct internal_vertex_name<raum::graph::Resource> {
    typedef multi_index::member<raum::graph::Resource, std::string, &raum::graph::Resource::name> type;
};

/// Allow the graph to build cities given only their names (filling in
/// the defaults for fields).
template <>
struct internal_vertex_constructor<raum::graph::Resource> {
    typedef vertex_from_name<raum::graph::Resource> type;
};

} // namespace graph
} // namespace boost

namespace raum::graph {

using ResourceGraphImpl = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Resource, boost::no_property>;

class ResourceGraph {
public:
    explicit ResourceGraph(rhi::RHIDevice* device);

    ResourceGraph() = delete;
    ResourceGraph(const ResourceGraph&) = delete;
    ResourceGraph& operator=(const ResourceGraph&) = delete;
    ResourceGraph(ResourceGraph&&) = delete;

    void addResource(std::string_view name, const BufferData& data);
    void mount(std::string_view name);
    void unmount(std::string_view name);

private:
    rhi::RHIDevice* _device{nullptr};
    ResourceGraphImpl _graph;
};

}