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

template <>
struct internal_vertex_name<raum::graph::Resource> {
    typedef multi_index::member<raum::graph::Resource, std::string, &raum::graph::Resource::name> type;
};

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

    void addBuffer(std::string_view name, const BufferData& data);
    void addBufferView(std::string_view name, const BufferViewData& data);
    void addImage(std::string_view name, const ImageData& data);
    void addImageView(std::string_view name, const ImageViewData& data);
    void mount(std::string_view name);
    void unmount(std::string_view name, uint64_t life);

private:
    rhi::RHIDevice* _device{nullptr};
    ResourceGraphImpl _graph;
};

}