#include "ResourceGraph.h"
#include "RHIBuffer.h"
#include "RHIBufferView.h"
#include "RHIDevice.h"
#include "RHIImage.h"
#include "RHIImageView.h"

using boost::add_vertex;
using boost::graph::find_vertex;
using boost::add_edge;
using raum::rhi::RHIBuffer;
using raum::rhi::RHIBufferView;
using raum::rhi::RHIDevice;
using raum::rhi::RHIImage;
using raum::rhi::RHIImageView;

namespace raum::graph {

ResourceGraph::ResourceGraph(RHIDevice* device) : _device(device) {
}

void ResourceGraph::addBuffer(std::string_view name, const BufferData& data) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].resource = data;
}

void ResourceGraph::addBufferView(std::string_view name, const BufferViewData& data) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].resource = data;
    add_edge(v, data.origin.data(), _graph);
}

void ResourceGraph::addImage(std::string_view name, const ImageData& data) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].resource = data;
}

void ResourceGraph::addImageView(std::string_view name, const raum::graph::ImageViewData& data) {
    const auto& v = add_vertex(name.data(), _graph);
    _graph[v].resource = data;
    add_edge(v, data.origin.data(), _graph);
}

void ResourceGraph::mount(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    _graph[v].life++;

    std::visit(overloaded{
                   [&](BufferData& data) {
                       data.buffer = _device->createBuffer(data.info);
                   },
                   [&](BufferViewData& data) {
                       const auto& v = *find_vertex(data.origin.data(), _graph);
                       const auto& originData = std::get<BufferData>(_graph[v].resource);
                       data.info.buffer = originData.buffer;
                       data.bufferView = _device->createBufferView(data.info);
                   },
                   [&](ImageData& data) {
                       data.image = _device->createImage(data.info);
                   },
                   [&](ImageViewData& data) {
                       const auto& v = *find_vertex(data.origin.data(), _graph);
                       const auto& originData = std::get<ImageData>(_graph[v].resource);
                       data.info.image = originData.image;
                       data.imageView = _device->createImageView(data.info);
                   },
                   [](auto&) {
                   }},
               _graph[v].resource);
}

void ResourceGraph::unmount(std::string_view name, uint64_t life) {
    const auto& v = *find_vertex(name.data(), _graph);
    auto& resource = _graph[v];
    if (resource.life < life) {
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
            resource.resource);
    }
}

} // namespace raum::graph