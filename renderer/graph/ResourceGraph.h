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

enum class Aspect : uint32_t {
    COLOR = rhi::AspectMask::COLOR,
    DEPTH = rhi::AspectMask::DEPTH,
    STENCIL = rhi::AspectMask::STENCIL,
};

struct SwapchainData {
    rhi::SwapchainPtr swapchain;
    std::map<uint8_t, rhi::ImagePtr> images;
    std::map<uint8_t, rhi::ImageViewPtr> imageViews;
};

struct Resource {
    std::string_view name{};
    ResourceResidency residency{ResourceResidency::DONT_CARE};
    rhi::AccessFlags access{rhi::AccessFlags::NONE};
    std::variant<BufferData, BufferViewData, ImageData, ImageViewData, SamplerData, SwapchainData> data;
    uint64_t life{0};
};
} // namespace raum::graph

namespace boost {
namespace graph {

template <>
struct internal_vertex_name<raum::graph::Resource> {
    typedef multi_index::member<raum::graph::Resource, std::string_view, &raum::graph::Resource::name> type;
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
    using VertexType = ResourceGraphImpl::vertex_descriptor;
    explicit ResourceGraph(rhi::RHIDevice* device);

    ResourceGraph() = delete;
    ResourceGraph(const ResourceGraph&) = delete;
    ResourceGraph& operator=(const ResourceGraph&) = delete;
    ResourceGraph(ResourceGraph&&) = delete;

    void addBuffer(std::string_view name, const BufferData& data);
    void addBuffer(std::string_view name, uint32_t size, rhi::BufferUsage usage);
    void addBufferView(std::string_view name, const BufferViewData& data);
    void addImage(std::string_view name, const rhi::ImageInfo& data);
    void addImage(std::string_view name, rhi::ImageUsage, uint32_t width, uint32_t height, rhi::Format format);
    void addImageView(std::string_view name, const ImageViewData& data);
    void addSampler(std::string_view name, const rhi::SamplerInfo& data);
    void import(std::string_view name, rhi::SwapchainPtr swapchain);
    void mount(std::string_view name);
    void unmount(std::string_view name, uint64_t life);
    void updateImage(std::string_view name, uint32_t width, uint32_t height);

    static VertexType null_vertex() { return boost::graph_traits<ResourceGraphImpl>::null_vertex(); }
    auto& impl() { return _graph; }

    bool contains(std::string_view name);
    const Resource& get(std::string_view name) const;
    Resource& get(std::string_view name);
    const Resource& getView(std::string_view name) const;
    Resource& getView(std::string_view name);

    const Resource& getAspectView(std::string_view name, Aspect aspect) const;
    Resource& getAspectView(std::string_view name, Aspect aspect);

    rhi::BufferPtr getBuffer(std::string_view name);
    rhi::BufferViewPtr getBufferView(std::string_view name);
    rhi::ImagePtr getImage(std::string_view name);
    rhi::ImageViewPtr getImageView(std::string_view name);
    rhi::SwapchainPtr getSwapchain(std::string_view name);

private:
    void addImageView(std::string_view name, const rhi::ImageInfo& info);
    rhi::RHIDevice* _device{nullptr};
    ResourceGraphImpl _graph;
    std::unordered_set<std::string, hash_string, std::equal_to<>> _names;
};

}