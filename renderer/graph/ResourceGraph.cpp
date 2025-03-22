#include "ResourceGraph.h"
#include <boost/graph/depth_first_search.hpp>
#include "RHIBuffer.h"
#include "RHIBufferView.h"
#include "RHIDevice.h"
#include "RHIImage.h"
#include "RHIImageView.h"
#include "RHISwapchain.h"

namespace raum::graph {
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

bool isDepthStencil(const rhi::Format& format) {
    return format >= rhi::Format::D16_UNORM && format <= rhi::Format::D32_SFLOAT_S8_UINT;
}

} // namespace

ResourceGraph::ResourceGraph(RHIDevice* device) : _device(device) {
}

void ResourceGraph::addBuffer(std::string_view name, const BufferData& data) {
    const auto& p = _names.emplace(name);
    if (p.second) {
        const auto& v = add_vertex(*p.first, _graph);
        _graph[v].data = data;
    }
}

void ResourceGraph::addBuffer(std::string_view name, uint32_t size, rhi::BufferUsage usage) {
    const auto& p = _names.emplace(name);
    if (p.second) {
        const auto& v = add_vertex(*p.first, _graph);
        _graph[v].data = BufferData{
            .info = {
                .bufferUsage = usage,
                .size = size,
            },
        };
    }
}

void ResourceGraph::addBufferView(std::string_view name, const BufferViewData& data) {
    const auto& p = _names.emplace(name);
    if (p.second) {
        const auto& v = add_vertex(*p.first, _graph);
        _graph[v].data = data;
        add_edge(data.origin, v, _graph);
    }
}

void ResourceGraph::addImageView(std::string_view name, const rhi::ImageInfo& info) {
    // ds/Depth , ds/Stencil
    if (isDepthStencil(info.format)) {
        ImageViewData depthView{
            .origin = std::string{name},
            .info = getDefaultViewInfo(info),
            .imageView = nullptr,
        };
        depthView.info.range.aspect = rhi::AspectMask::DEPTH;
        addImageView("Depth", depthView);

        ImageViewData stencilView{
            .origin = std::string{name},
            .info = getDefaultViewInfo(info),
            .imageView = nullptr,
        };
        stencilView.info.range.aspect = rhi::AspectMask::STENCIL;
        addImageView("Stencil", stencilView);
    }

    ImageViewData view{
        .origin = std::string{name},
        .info = getDefaultViewInfo(info),
        .imageView = nullptr,
    };
    addImageView(name, view);
}

void ResourceGraph::addImage(std::string_view name, const rhi::ImageInfo& info) {
    const auto& p = _names.emplace(name);
    if (p.second) {
        const auto& v = add_vertex(*p.first, _graph);
        _graph[v].data = ImageData{info};
        addImageView(name, info);
    }
}

void ResourceGraph::addImage(std::string_view name, rhi::ImageUsage usage, uint32_t width, uint32_t height, rhi::Format format) {
    const auto& p = _names.emplace(name);
    if (p.second) {
        const auto& v = add_vertex(*p.first, _graph);
        rhi::ImageInfo info{
            .type = rhi::ImageType::IMAGE_2D,
            .usage = usage,
            .format = format,
            .sliceCount = 1,
            .mipCount = 1,
            .extent = {width, height, 1},
        };
        _graph[v].data = ImageData{info};

        addImageView(name, info);
    }
}

void ResourceGraph::addImageView(std::string_view name, const ImageViewData& data) {
    const auto& p = _names.emplace(data.origin + "/" + std::string{name});
    if (p.second) {
        const auto& v = add_vertex(*p.first, _graph);
        _graph[v].data = data;
        add_edge(data.origin, v, _graph);
    }
}

void ResourceGraph::addSampler(std::string_view name, const rhi::SamplerInfo& info) {
    const auto& p = _names.emplace(name);
    if (p.second) {
        const auto& v = add_vertex(*p.first, _graph);
        _graph[v].data = SamplerData{info};
    }
}

void ResourceGraph::import(std::string_view name, rhi::SwapchainPtr swapchain) {
    const auto& p = _names.emplace(name);
    if (p.second) {
        const auto& v = add_vertex(name, _graph);
        _graph[v].data = SwapchainData{swapchain};
        _graph[v].residency = ResourceResidency::SWAPCHAIN;
    }
}

void ResourceGraph::updateImage(std::string_view name, uint32_t width, uint32_t height) {
    auto v = *find_vertex(name, _graph);
    auto& image = std::get<ImageData>(_graph[v].data);
    if (image.info.extent.x != width || image.info.extent.y != height) {
        unmount(name, std::numeric_limits<uint64_t>::max());
        image.info.extent.x = width;
        image.info.extent.y = height;
    }
}

void ResourceGraph::mount(std::string_view name) {
    auto v = *find_vertex(name, _graph);
    _graph[v].life++;

    std::visit(overloaded{
                   [&](BufferData& data) {
                       if (!data.buffer) {
                           data.buffer = rhi::BufferPtr(_device->createBuffer(data.info));
                       }
                   },
                   [&](BufferViewData& data) {
                       if (!data.bufferView) {
                           const auto& v = *find_vertex(data.origin, _graph);
                           const auto& originData = std::get<BufferData>(_graph[v].data);
                           data.info.buffer = originData.buffer.get();
                           data.bufferView = rhi::BufferViewPtr(_device->createBufferView(data.info));
                       }
                   },
                   [&](ImageData& data) {
                       if (!data.image) {
                           data.image = rhi::ImagePtr(_device->createImage(data.info));

                           // depth/stencil seperate aspect view
                           if (isDepthStencil(data.info.format)) {
                               auto& depthViewResource = getAspectView(name, Aspect::DEPTH);
                               auto& depthView = std::get<ImageViewData>(depthViewResource.data);
                               depthView.info.image = data.image.get();
                               depthView.imageView = rhi::ImageViewPtr(_device->createImageView(depthView.info));

                               auto& stencilViewResource = getAspectView(name, Aspect::STENCIL);
                               auto& stencilView = std::get<ImageViewData>(stencilViewResource.data);
                               stencilView.info.image = data.image.get();
                               stencilView.imageView = rhi::ImageViewPtr(_device->createImageView(stencilView.info));
                           }

                           auto& viewResource = getView(name);
                           auto& imageView = std::get<ImageViewData>(viewResource.data);
                           imageView.info.image = data.image.get();
                           imageView.imageView = rhi::ImageViewPtr(_device->createImageView(imageView.info));
                       }
                   },
                   [&](ImageViewData& data) {
                       if (!data.imageView) {
                           const auto& v = *find_vertex(data.origin, _graph);
                           const auto& originData = std::get<ImageData>(_graph[v].data);
                           data.info.image = originData.image.get();
                           data.imageView = rhi::ImageViewPtr(_device->createImageView(data.info));
                       }
                   },
                   [&](SwapchainData& data) {
                       auto& swapchain = data.swapchain;
                       if (!swapchain->imageValid(0)) {
                           for (uint32_t i = 0; i < swapchain->imageCount(); ++i) {
                               auto index = static_cast<uint8_t>(i);
                               auto imagePtr = rhi::ImagePtr(swapchain->allocateImage(index));
                               data.images.emplace(index, imagePtr);

                               const auto& imageInfo = imagePtr->info();
                               rhi::ImageViewInfo viewInfo = getDefaultViewInfo(imageInfo);
                               viewInfo.image = imagePtr.get();
                               data.imageViews.emplace(index, rhi::ImageViewPtr(_device->createImageView(viewInfo)));
                           }
                       }
                   },
                   [](auto&) {
                   }},
               _graph[v].data);
}

struct UnmountVisitor : public boost::dfs_visitor<> {
    // make sure view unmounting comes before resource itself
    void finish_vertex(ResourceGraph::VertexType v, const ResourceGraphImpl& _) {
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

    ResourceGraphImpl& g;
};

void ResourceGraph::unmount(std::string_view name, uint64_t life) {
    const auto& v = *find_vertex(name, _graph);
    auto& resource = _graph[v];
    if (resource.life < life) {
        auto indexMap = boost::get(boost::vertex_index, _graph);
        auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);
        UnmountVisitor uv{{}, _graph};
        boost::depth_first_visit(_graph, v, uv, colorMap);
    }
}

bool ResourceGraph::contains(std::string_view name) {
    return find_vertex(name, _graph).has_value();
}

const Resource& ResourceGraph::get(std::string_view name) const {
    auto v = *find_vertex(name, _graph);
    return _graph[v];
}

Resource& ResourceGraph::get(std::string_view name) {
    auto v = *find_vertex(name, _graph);
    return _graph[v];
}

const Resource& ResourceGraph::getView(std::string_view name) const {
    auto v = *find_vertex(name, _graph);
    size_t res{INVALID_VERTEX};
    for (const auto& e : make_iterator_range(out_edges(v, _graph))) {
        if (_graph[e.m_target].name.length() < (name.length() * 2 + 1)) {
            continue;
        }
        auto childName = std::string_view(_graph[e.m_target].name).substr(name.length() + 1, name.length());
        if (name == childName) {
            res = e.m_target;
            break;
        }
    }
    raum_check(res != INVALID_VERTEX, "can't find resource view: {}/{}", name, name);
    return _graph[res];
}

Resource& ResourceGraph::getView(std::string_view name) {
    auto v = *find_vertex(name, _graph);
    size_t res{INVALID_VERTEX};
    for (const auto& e : make_iterator_range(out_edges(v, _graph))) {
        if (_graph[e.m_target].name.length() < (name.length() * 2 + 1)) {
            continue;
        }
        auto childName = std::string_view(_graph[e.m_target].name).substr(name.length() + 1, name.length());
        if (name == childName) {
            res = e.m_target;
            break;
        }
    }
    raum_check(res != INVALID_VERTEX, "can't find resource view: {}/{}", name, name);
    return _graph[res];
}

Resource& ResourceGraph::getAspectView(std::string_view name, Aspect aspect) {
    auto v = *find_vertex(name, _graph);
    size_t res{INVALID_VERTEX};
    for (const auto& e : make_iterator_range(out_edges(v, _graph))) {
        // name = "ds"
        // "ds/Depth" -> depth view
        auto childName = std::string_view(_graph[e.m_target].name)
                             .substr(
                                 name.length() + 1,
                                 _graph[e.m_target].name.length() - name.length() - 1);
        if (aspect == Aspect::DEPTH && childName == "Depth") {
            res = e.m_target;
            break;
        } else if (aspect == Aspect::STENCIL && childName == "Stencil") {
            res = e.m_target;
            break;
        }
    }
    raum_check(res != INVALID_VERTEX, "can't find resource view: {}/{}", name, name);
    return _graph[res];
}

const Resource& ResourceGraph::getAspectView(std::string_view name, Aspect aspect) const {
    auto v = *find_vertex(name, _graph);
    size_t res{INVALID_VERTEX};
    for (const auto& e : make_iterator_range(out_edges(v, _graph))) {
        // name = "ds"
        // "ds/Depth" -> depth view
        auto childName = std::string_view(_graph[e.m_target].name)
                             .substr(
                                 name.length() + 1,
                                 _graph[e.m_target].name.length() - name.length() - 1);
        if (aspect == Aspect::DEPTH && childName == "Depth") {
            res = e.m_target;
            break;
        } else if (aspect == Aspect::STENCIL && childName == "Stencil") {
            res = e.m_target;
            break;
        }
    }
    raum_check(res != INVALID_VERTEX, "can't find resource view: {}/{}", name, name);
    return _graph[res];
}

rhi::BufferPtr ResourceGraph::getBuffer(std::string_view name) {
    Resource& res = get(name);
    if (std::holds_alternative<BufferData>(res.data)) {
        return std::get<BufferData>(res.data).buffer;
    }
    raum_unreachable();
    return nullptr;
}

rhi::BufferViewPtr ResourceGraph::getBufferView(std::string_view name) {
    Resource& res = get(name);
    if (std::holds_alternative<BufferViewData>(res.data)) {
        return std::get<BufferViewData>(res.data).bufferView;
    }
    raum_unreachable();
    return nullptr;
}

rhi::ImagePtr ResourceGraph::getImage(std::string_view name) {
    Resource& res = get(name);
    if (std::holds_alternative<ImageData>(res.data)) {
        return std::get<ImageData>(res.data).image;
    } else if (std::holds_alternative<SwapchainData>(res.data)) {
        auto& swapchainData = std::get<SwapchainData>(res.data);
        return swapchainData.images.at(swapchainData.swapchain->imageIndex());
    }
    raum_unreachable();
    return nullptr;
}

rhi::ImageViewPtr ResourceGraph::getImageView(std::string_view name) {
    Resource& res = get(name);
    if (std::holds_alternative<ImageViewData>(res.data)) {
        return std::get<ImageViewData>(res.data).imageView;
    } else if (std::holds_alternative<ImageData>(res.data)) {
        auto& viewRes = getView(name);
        return std::get<ImageViewData>(viewRes.data).imageView;
    } else if (std::holds_alternative<SwapchainData>(res.data)) {
        auto& swapchainData = std::get<SwapchainData>(res.data);
        return swapchainData.imageViews.at(swapchainData.swapchain->imageIndex());
    }
    raum_unreachable();
    return nullptr;
}

} // namespace raum::graph