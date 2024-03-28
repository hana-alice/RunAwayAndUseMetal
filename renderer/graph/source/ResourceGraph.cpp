#include "ResourceGraph.h"
#include "RHIBuffer.h"
#include "RHIBufferView.h"
#include "RHIDevice.h"
#include "RHIImage.h"
#include "RHIImageView.h"
#include <boost/graph/depth_first_search.hpp>

namespace raum::graph {
using boost::add_edge;
using boost::add_vertex;
using boost::graph::find_vertex;
using boost::out_edges;
using boost::make_iterator_range;
using raum::rhi::RHIBuffer;
using raum::rhi::RHIBufferView;
using raum::rhi::RHIDevice;
using raum::rhi::RHIImage;
using raum::rhi::RHIImageView;


namespace {
rhi::ImageViewInfo getDefaultViewInfo(const rhi::ImageInfo& info) {
    rhi::ImageViewInfo viewInfo{};
    viewInfo.format = info.format;
    viewInfo.range = {
        .aspect = info.format >= rhi::Format::D16_UNORM && info.format <= rhi::Format::D32_SFLOAT_S8_UINT ? rhi::AspectMask::COLOR : rhi::AspectMask::DEPTH | rhi::AspectMask::STENCIL,
        .firstSlice = 0,
        .sliceCount = info.sliceCount,
        .firstMip = 0,
        .mipCount = info.mipCount,
    };
    switch (info.type) {
        case rhi::ImageType::IMAGE_1D: {
            viewInfo.type = info.sliceCount > 1 ? rhi::ImageViewType::IMAGE_VIEW_1D_ARRAY : rhi::ImageViewType::IMAGE_VIEW_1D;
        }
        case rhi::ImageType::IMAGE_2D: {
            viewInfo.type = info.sliceCount > 1 ? rhi::ImageViewType::IMAGE_VIEW_2D_ARRAY : rhi::ImageViewType::IMAGE_VIEW_2D;
        }
        case rhi::ImageType::IMAGE_3D: {
            viewInfo.type = rhi::ImageViewType::IMAGE_VIEW_3D;
        }
        default:
            break;
    }
    return viewInfo;
}
} // namespace

ResourceGraph::ResourceGraph(RHIDevice* device) : _device(device) {
}

void ResourceGraph::addBuffer(std::string_view name, const BufferData& data) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].data = data;
}

void ResourceGraph::addBufferView(std::string_view name, const BufferViewData& data) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].data = data;
    add_edge(v, data.origin.data(), _graph);
}

void ResourceGraph::addImage(std::string_view name, const rhi::ImageInfo& info) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].data = ImageData{info};

    ImageViewData view{
        .origin = std::string{name},
        .info = getDefaultViewInfo(info),
        .imageView = nullptr,
    };
    addImageView(_graph[v].name + "/" + name.data(), view);
}

void ResourceGraph::addImage(std::string_view name, rhi::ImageUsage usage, uint32_t width, uint32_t height, rhi::Format format) {
    const auto& v = add_vertex(name.data(), _graph);
    rhi::ImageInfo info{
        .type = rhi::ImageType::IMAGE_2D,
        .usage = usage,
        .format = format,
        .sliceCount = 1,
        .mipCount = 1,
        .extent = {width, height, 1},
    };
    _graph[v].data = ImageData{info};

    ImageViewData view{
        .origin = std::string{name},
        .info = getDefaultViewInfo(info),
        .imageView = nullptr,
    };
    addImageView(_graph[v].name + "/" + name.data(), view);
}

void ResourceGraph::addImageView(std::string_view name, const raum::graph::ImageViewData& data) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].data = data;
    add_edge(v, data.origin.data(), _graph);
}

void ResourceGraph::mount(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    _graph[v].life++;

    std::visit(overloaded{
                   [&](BufferData& data) {
                       if (!data.buffer) {
                           data.buffer = _device->createBuffer(data.info);
                       }
                   },
                   [&](BufferViewData& data) {
                       if (!data.bufferView) {
                           const auto& v = *find_vertex(data.origin.data(), _graph);
                           const auto& originData = std::get<BufferData>(_graph[v].data);
                           data.info.buffer = originData.buffer;
                           data.bufferView = _device->createBufferView(data.info);
                       }
                   },
                   [&](ImageData& data) {
                       if (!data.image) {
                           data.image = _device->createImage(data.info);

                           auto& viewResource = getView(name);
                           auto& imageView = std::get<ImageViewData>(viewResource.data);
                           imageView.info.image = data.image;
                           imageView.imageView = _device->createImageView(imageView.info);
                       }
                   },
                   [&](ImageViewData& data) {
                       if (!data.imageView) {
                           const auto& v = *find_vertex(data.origin.data(), _graph);
                           const auto& originData = std::get<ImageData>(_graph[v].data);
                           data.info.image = originData.image;
                           data.imageView = _device->createImageView(data.info);
                       }
                   },
                   [](auto&) {
                   }},
               _graph[v].data);
}

struct UnmountVisitor : public boost::dfs_visitor<> {
    // make sure view unmounting comes before resource itself
    void finish_vertex(ResourceGraph::VertexType v, const ResourceGraphImpl& g) {
        auto& resource = g[v];
        std::visit(
            overloaded{
                [](BufferData& data) {
                    delete data.buffer;
                    data.buffer = nullptr;
                },
                [](BufferViewData& data) {
                    delete data.bufferView;
                    data.bufferView = nullptr;
                },
                [](ImageData& data) {
                    delete data.image;
                    data.image = nullptr;
                },
                [](ImageViewData& data) {
                    delete data.imageView;
                    data.imageView = nullptr;
                },
                [](auto&) {
                },
            },
            resource.data);
    }
};

void ResourceGraph::unmount(std::string_view name, uint64_t life) {
    const auto& v = *find_vertex(name.data(), _graph);
    auto& resource = _graph[v];
    if (resource.life < life) {
        auto indexMap = boost::get(boost::vertex_index, _graph);
        auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);
        UnmountVisitor uv{};
        boost::depth_first_visit(_graph, v, uv, colorMap);
    }
}

bool ResourceGraph::contains(std::string_view name) {
    return find_vertex(name.data(), _graph).has_value();
}

const Resource& ResourceGraph::get(std::string_view name) const {
    auto v = *find_vertex(name.data(), _graph);
    return _graph[v];
}

Resource& ResourceGraph::get(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    return _graph[v];
}

const Resource& ResourceGraph::getView(std::string_view name) const {
    auto v = *find_vertex(name.data(), _graph);
    size_t res{INVALID_VERTEX};
    for (const auto& e : make_iterator_range(out_edges(v, _graph))) {
        if (_graph[e.m_target].name == name) {
            res = e.m_target;
        }
    }
    raum_check(res != INVALID_VERTEX, "can't find resource view: {}/{}", name, name);
    return _graph[v];
}

Resource& ResourceGraph::getView(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    size_t res{INVALID_VERTEX};
    for (const auto& e : make_iterator_range(out_edges(v, _graph))) {
        if (_graph[e.m_target].name == name) {
            res = e.m_target;
        }
    }
    raum_check(res != INVALID_VERTEX, "can't find resource view: {}/{}", name, name);
    return _graph[v];
}

} // namespace raum::graph