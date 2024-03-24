#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "GraphTypes.h"

namespace raum::rhi {
class RHIDevice;
}

namespace raum::graph {

enum class ResourceResidency : uint8_t {
    DONT_CARE,
    PERSISTENT,
    EXTERNAL,
    SWAPCHAIN,
};

struct Resource {
    std::string name{};
    ResourceResidency residency{ResourceResidency::DONT_CARE};
    rhi::AccessFlags access{rhi::AccessFlags::NONE};
    std::variant<BufferData, BufferViewData, ImageData, ImageViewData> data;
    uint64_t life{0};
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
    void addImage(std::string_view name, const rhi::ImageInfo& data);
    void addImage(std::string_view name, rhi::ImageUsage, uint32_t width, uint32_t height, rhi::Format format);
    void addImageView(std::string_view name, const ImageViewData& data);
    void mount(std::string_view name);
    void unmount(std::string_view name, uint64_t life);

    bool contains(std::string_view name);
    const Resource& get(std::string_view name) const;
    Resource& get(std::string_view name);

private:
    rhi::RHIDevice* _device{nullptr};
    ResourceGraphImpl _graph;
};

}