#include "ResourceGraph.h"
#include "RHIBuffer.h"
#include "RHIBufferView.h"
#include "RHIDevice.h"
#include "RHIImage.h"
#include "RHIImageView.h"

using boost::add_vertex;
using boost::graph::find_vertex;
using raum::rhi::RHIBuffer;
using raum::rhi::RHIBufferView;
using raum::rhi::RHIDevice;
using raum::rhi::RHIImage;
using raum::rhi::RHIImageView;

namespace raum::graph {

ResourceGraph::ResourceGraph(RHIDevice* device) : _device(device) {
}

void ResourceGraph::addResource(std::string_view name, const BufferData& data) {
    std::ignore = add_vertex(name.data(), _graph);
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

void ResourceGraph::unmount(std::string_view name) {
}

} // namespace raum::graph