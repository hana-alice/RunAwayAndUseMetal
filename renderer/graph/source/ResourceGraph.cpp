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
        .aspect = info.format >= rhi::Format::D16_UNORM && info.format <= rhi::Format::D32_SFLOAT_S8_UINT ? rhi::AspectMask::DEPTH | rhi::AspectMask::STENCIL : rhi::AspectMask::COLOR,
        .firstSlice = 0,
        .sliceCount = info.sliceCount,
        .firstMip = 0,
        .mipCount = info.mipCount,
    };
    switch (info.type) {
        case rhi::ImageType::IMAGE_1D: {
            viewInfo.type = info.sliceCount > 1 ? rhi::ImageViewType::IMAGE_VIEW_1D_ARRAY : rhi::ImageViewType::IMAGE_VIEW_1D;
            break;
        }
        case rhi::ImageType::IMAGE_2D: {
            viewInfo.type = info.sliceCount > 1 ? rhi::ImageViewType::IMAGE_VIEW_2D_ARRAY : rhi::ImageViewType::IMAGE_VIEW_2D;
            break;
        }
        case rhi::ImageType::IMAGE_3D: {
            viewInfo.type = rhi::ImageViewType::IMAGE_VIEW_3D;
            break;
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

void ResourceGraph::addBuffer(std::string_view name, uint32_t size, rhi::BufferUsage usage) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].data = BufferData{
        .info = {
            .bufferUsage = usage,
            .size = size,
        },
    };
}

void ResourceGraph::addBufferView(std::string_view name, const BufferViewData& data) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].data = data;
    add_edge( data.origin.data(), v, _graph);
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
    add_edge(data.origin.data(), v, _graph);
}

void ResourceGraph::import(std::string_view name, rhi::SwapchainPtr swapchain) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].data = swapchain;
    _graph[v].residency = ResourceResidency::SWAPCHAIN;
}

void ResourceGraph::mount(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    _graph[v].life++;

    std::visit(overloaded{
                   [&](BufferData& data) {
                       if (!data.buffer) {
                           data.buffer = rhi::BufferPtr(_device->createBuffer(data.info));
                       }
                   },
                   [&](BufferViewData& data) {
                       if (!data.bufferView) {
                           const auto& v = *find_vertex(data.origin.data(), _graph);
                           const auto& originData = std::get<BufferData>(_graph[v].data);
                           data.info.buffer = originData.buffer.get();
                           data.bufferView = rhi::BufferViewPtr(_device->createBufferView(data.info));
                       }
                   },
                   [&](ImageData& data) {
                       if (!data.image) {
                           data.image = rhi::ImagePtr(_device->createImage(data.info));

                           auto& viewResource = getView(name);
                           auto& imageView = std::get<ImageViewData>(viewResource.data);
                           imageView.info.image = data.image.get();
                           imageView.imageView = rhi::ImageViewPtr(_device->createImageView(imageView.info));
                       }
                   },
                   [&](ImageViewData& data) {
                       if (!data.imageView) {
                           const auto& v = *find_vertex(data.origin.data(), _graph);
                           const auto& originData = std::get<ImageData>(_graph[v].data);
                           data.info.image = originData.image.get();
                           data.imageView = rhi::ImageViewPtr(_device->createImageView(data.info));
                       }
                   },
                   [&](rhi::SwapchainPtr data) {
                       if(!boost::out_degree(v, _graph)) {
                           for(size_t i = 0; i < data->imageCount(); ++i) {
                               std::string imageName{name};
                               imageName.append("/" + std::to_string(i));
                               const auto& vert = add_vertex(imageName, _graph);

                               rhi::ImageInfo info{
                                   .type = rhi::ImageType::IMAGE_2D,
                                   .usage = rhi::ImageUsage::COLOR_ATTACHMENT | rhi::ImageUsage::TRANSFER_DST,
                                   .format = data->format(),
                                   .sliceCount = 1,
                                   .mipCount = 1,
                                   .extent = {data->width(), data->height(), 1},
                               };

                               auto image = rhi::ImagePtr(data->allocateImage(i));
                               _graph[vert].data = ImageData{info, image};
                               add_edge(name.data(), vert, _graph);

                               rhi::ImageViewInfo viewInfo = getDefaultViewInfo(info);
                               viewInfo.image = image.get();
                               imageName.append("/" + std::to_string(i));
                               const auto& viewVert = add_vertex(imageName, _graph);
                               _graph[viewVert].data = ImageViewData{
                                   _graph[vert].name,
                                   viewInfo,
                                   rhi::ImageViewPtr(_device->createImageView(viewInfo)),
                               };
                               add_edge( _graph[vert].name, viewVert, _graph);
                           }
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
                    data.buffer.reset();
                },
                [](BufferViewData& data) {
                    data.bufferView.reset();
                },
                [](ImageData& data) {
                    data.image.reset();
                },
                [](ImageViewData& data) {
                    data.imageView.reset();
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
    std::string viewName{name};
    viewName.append("/");
    viewName.append(name);
    for (const auto& e : make_iterator_range(out_edges(v, _graph))) {
        if (_graph[e.m_target].name == viewName) {
            res = e.m_target;
            break;
        }
    }
    raum_check(res != INVALID_VERTEX, "can't find resource view: {}/{}", name, name);
    return _graph[res];
}

Resource& ResourceGraph::getView(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    size_t res{INVALID_VERTEX};
    std::string viewName{name};
    viewName.append("/");
    viewName.append(name);
    for (const auto& e : make_iterator_range(out_edges(v, _graph))) {
        if (_graph[e.m_target].name == viewName) {
            res = e.m_target;
            break;
        }
    }
    raum_check(res != INVALID_VERTEX, "can't find resource view: {}/{}", name, name);
    return _graph[res];
}

rhi::BufferPtr ResourceGraph::getBuffer(std::string_view name) {
    Resource& res = get(name);
    if(std::holds_alternative<BufferData>(res.data)) {
        return std::get<BufferData>(res.data).buffer;
    }
    return nullptr;
}

rhi::BufferViewPtr ResourceGraph::getBufferView(std::string_view name) {
    Resource& res = get(name);
    if(std::holds_alternative<BufferViewData>(res.data)) {
        return std::get<BufferViewData>(res.data).bufferView;
    }
    return nullptr;
}

rhi::ImagePtr ResourceGraph::getImage(std::string_view name) {
    Resource& res = get(name);
    if(std::holds_alternative<ImageData>(res.data)) {
        return std::get<ImageData>(res.data).image;
    } else if(std::holds_alternative<rhi::SwapchainPtr>(res.data)) {
        std::string imageName(name);
        imageName.append("/" + std::to_string(std::get<rhi::SwapchainPtr>(res.data)->imageIndex()));
        auto v = *find_vertex(imageName, _graph);
        return std::get<ImageData>(_graph[v].data).image;
    }
    return nullptr;
}

rhi::ImageViewPtr ResourceGraph::getImageView(std::string_view name) {
    Resource& res = get(name);
    if(std::holds_alternative<ImageViewData>(res.data)) {
        return std::get<ImageViewData>(res.data).imageView;
    } else if (std::holds_alternative<ImageData>(res.data)) {
        auto& viewRes = getView(name);
        return std::get<ImageViewData>(viewRes.data).imageView;
    } else if (std::holds_alternative<rhi::SwapchainPtr>(res.data)) {
        std::string viewName(name);
        viewName.append("/" + std::to_string(std::get<rhi::SwapchainPtr>(res.data)->imageIndex()));
        viewName.append("/" + std::to_string(std::get<rhi::SwapchainPtr>(res.data)->imageIndex()));
        auto v = *find_vertex(viewName, _graph);
        return std::get<ImageViewData>(_graph[v].data).imageView;
    }
    return nullptr;
}

} // namespace raum::graph