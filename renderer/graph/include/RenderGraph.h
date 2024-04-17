#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "Camera.h"
#include "GraphTypes.h"
#include "Scene.h"


namespace raum::graph {
struct Pass {
    std::string name{};
    std::variant<RenderPassData, SubRenderPassData, ComputePassData, CopyPassData, RenderQueueData> data;
};
} // namespace raum::graph

namespace boost {
namespace graph {

template <>
struct internal_vertex_name<raum::graph::Pass> {
    typedef multi_index::member<raum::graph::Pass, std::string, &raum::graph::Pass::name> type;
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
    //RenderQueue& addScene(scene::Scene* scene);
    RenderQueue& setViewport(int32_t x, int32_t y, uint32_t w, uint32_t h, float minDepth, float maxDepth);
    RenderQueue& addUniformBuffer(std::string_view name, std::string_view bindingName);

private:
    RenderGraphImpl::vertex_descriptor _id{0};
    RenderGraphImpl& _graph;
};

class RenderPass {
public:
    RenderPass(RenderGraphImpl::vertex_descriptor id, RenderGraphImpl& graph) : _id(id), _graph(graph) {}
    RenderPass(const RenderPass& rhs) : _id(rhs._id), _graph(rhs._graph) {}
    RenderPass& operator=(const RenderPass& rhs) {
        _id = rhs._id;
        _graph = rhs._graph;
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

private:
    ComputePassData& _data;
};

class CopyPass {
public:
    CopyPass(CopyPassData& data) : _data(data) {}
    CopyPass(const CopyPass& rhs) : _data(rhs._data) {}
    CopyPass& operator=(const CopyPass& rhs) {
        _data = rhs._data;
        return *this;
    }

    CopyPass(CopyPass&& rhs) = delete;
    ~CopyPass() = default;

    CopyPass& addPair(const CopyPair&);
    CopyPass& uploadBuffer(const UploadPair&);

private:
    CopyPassData& _data;
};

class RenderGraph {
public:
    RenderGraph() = default;
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

private:
    RenderGraphImpl _graph;
};

} // namespace raum::graph