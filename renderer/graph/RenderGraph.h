#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "Camera.h"
#include "GraphTypes.h"
#include "Scene.h"


namespace raum::graph {
struct Pass {
    std::string_view name;
    std::variant<RenderPassData, SubRenderPassData, ComputePassData, CopyPassData, RenderQueueData> data;
};
} // namespace raum::graph

namespace boost {
namespace graph {

template <>
struct internal_vertex_name<raum::graph::Pass> {
    typedef multi_index::member<raum::graph::Pass, std::string_view, &raum::graph::Pass::name> type;
};

template <>
struct internal_vertex_constructor<raum::graph::Pass> {
    typedef vertex_from_name<raum::graph::Pass> type;
};

} // namespace graph
} // namespace boost

namespace raum::graph {

using RenderGraphImpl = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, Pass, boost::no_property>;

class RenderQueue {
public:
    RenderQueue() = delete;
    RenderQueue(RenderGraphImpl ::vertex_descriptor renderPassID, RenderGraphImpl& graph) : _id(renderPassID), _graph(graph){};

    RenderQueue& addCamera(scene::Camera* camera);
    //    RenderQueue& addQuad();
    //RenderQueue& addScene(scene::DIrector* scene);
    RenderQueue& setViewport(int32_t x, int32_t y, uint32_t w, uint32_t h, float minDepth, float maxDepth);
    RenderQueue& addUniformBuffer(std::string_view name, std::string_view bindingName);
    RenderQueue& addSampledImage(std::string_view name, std::string_view bindingName);
    RenderQueue& addSampler(std::string_view name, std::string_view bindingName);

private:
    RenderGraphImpl::vertex_descriptor _id{0};
    RenderGraphImpl& _graph;
};

class RenderPass {
public:
    RenderPass(RenderGraphImpl::vertex_descriptor id, RenderGraphImpl& graph, TransparentUnorderedSet& names) : _id(id), _graph(graph), _names(names) {}
    RenderPass(const RenderPass& rhs) : _id(rhs._id), _graph(rhs._graph), _names(rhs._names) {}
    RenderPass& operator=(const RenderPass& rhs) {
        _id = rhs._id;
        _graph = rhs._graph;
        _names = rhs._names;
        return *this;
    }
    RenderPass(RenderPass&& rhs) = delete;
    ~RenderPass() = default;

    RenderPass& addColor(std::string_view name, LoadOp loadOp, StoreOp storeOp, const ClearValue& color);
    RenderPass& addDepthStencil(std::string_view name, LoadOp loadOp, StoreOp storeOp, LoadOp stencilLoad, StoreOp stencilStore, float clearDepth, uint32_t clearStencil);
    RenderPass& addShadingRate(std::string_view name);

    RenderQueue addQueue(std::string_view name);

private:
    RenderGraphImpl::vertex_descriptor _id{0};
    RenderGraphImpl& _graph;
    TransparentUnorderedSet& _names;
};

class ComputePass {
public:
    ComputePass(ComputePassData& data) : _data(data) {}
    ComputePass(const ComputePass& rhs) : _data(rhs._data) {}
    ComputePass& operator=(const ComputePass& rhs) {
        _data = rhs._data;
        return *this;
    }

    ComputePass(ComputePass&& rhs) = delete;
    ~ComputePass() = default;

    ComputePass& addResource(std::string_view name, std::string_view bindingName, Access access);
    ComputePass& setPhase(std::string_view phase);

private:
    ComputePassData& _data;
};

class CopyPass {
public:
    CopyPass(CopyPassData& data, rhi::DevicePtr device) : _data(data),_device(device) {}
    CopyPass(const CopyPass& rhs) : _data(rhs._data), _device(rhs._device) {}
    CopyPass& operator=(const CopyPass& rhs) {
        _data = rhs._data;
        _device = rhs._device;
        return *this;
    }

    CopyPass(CopyPass&& rhs) = delete;
    ~CopyPass() = default;

    CopyPass& addPair(const CopyPair&);
    CopyPass& uploadBuffer(const void* const data, uint32_t size, std::string_view name, uint32_t dstOffset);
    CopyPass& fill(uint32_t value, uint32_t size, std::string_view name, uint32_t dstOffset);

private:
    CopyPassData& _data;
    rhi::DevicePtr _device;
};

class RenderGraph {
public:
    RenderGraph(rhi::DevicePtr);
    RenderGraph(const RenderGraph&) = delete;
    RenderGraph& operator=(const RenderGraph&) = delete;
    RenderGraph(RenderGraph&&) = delete;

    ~RenderGraph() = default;

    RenderPass addRenderPass(std::string_view name);
    ComputePass addComputePass(std::string_view name);
    CopyPass addCopyPass(std::string_view name);

    void clear();

    auto& impl() { return _graph; }

    using VertexType = RenderGraphImpl::vertex_descriptor;
    static VertexType null_vertex() { return RenderGraphImpl::null_vertex(); }

private:
    RenderGraphImpl _graph;
    rhi::DevicePtr _device;
   TransparentUnorderedSet _names; // pass queue names
   TransparentUnorderedSet _resNames; // rendering resource name
};

} // namespace raum::graph