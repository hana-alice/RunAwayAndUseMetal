#include "Graphs.h"
#include <boost/graph/depth_first_search.hpp>
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIComputeEncoder.h"
#include "RHIRenderEncoder.h"
#include "RHIDevice.h"

namespace raum::graph {

namespace {
    static std::unordered_map<rhi::RenderPassInfo, rhi::RenderPassPtr, rhi::RHIHash<rhi::RenderPassInfo>> _renderPassMap;
    static std::unordered_map<rhi::FrameBufferInfo, rhi::FrameBufferPtr, rhi::RHIHash<rhi::FrameBufferInfo>> _frameBufferMap;

    rhi::RenderPassPtr getOrCreateRenderPass(const rhi::RenderPassInfo& rpInfo, rhi::DevicePtr device) {
        if (!_renderPassMap[rpInfo]) {
            _renderPassMap[rpInfo] = rhi::RenderPassPtr(device->createRenderPass(rpInfo));
        }
        return _renderPassMap[rpInfo];
    }

    rhi::FrameBufferPtr getOrCreateFrameBuffer(const rhi::FrameBufferInfo& fbInfo, rhi::DevicePtr device) {
        if (!_frameBufferMap[fbInfo]) {
			_frameBufferMap[fbInfo] = rhi::FrameBufferPtr(device->createFrameBuffer(fbInfo));
		}
		return _frameBufferMap[fbInfo];
    }
}

void collectRenderables(std::map<uint32_t, scene::RenderablePtr>& renderables, const SceneGraph& sg) {
    const auto& graph = sg.impl();
    for (auto v : boost::make_iterator_range(boost::vertices(graph))) {
        if (std::holds_alternative<ModelNode>(graph[v].sceneNodeData)) {
            const auto& modelNode = std::get<ModelNode>(graph[v].sceneNodeData);
            for (auto& mesh : modelNode.model->meshes) {
                renderables.emplace(mesh->materialID, mesh);
            }
        }
    }
}

void getOrMakeRenderPass(RenderPassData& renderpassData, ResourceGraph& resg, rhi::DevicePtr device) {
    rhi::RenderPassInfo rpInfo{};

    device->createRenderPass(rpInfo);
}

struct PreProcessVisitor : public boost::dfs_visitor<> {

    void discover_vertex(const RenderGraph::VertexType v, RenderGraphImpl& g) {
        std::visit(overloaded{
                    [](RenderPassData& pass) {
                        
                    },
                    [&](RenderQueueData& queueData) {
                        collectRenderables(queueData.renderables, _sg);
                    },
                    [](auto _) {

                    },
                },
                g[v].data);
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
    }

    SceneGraph& _sg;
    rhi::DevicePtr _device;
};

struct RenderGraphVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](RenderPassData& renderPass) {
                           _renderEncoder = std::shared_ptr<rhi::RHIRenderEncoder>(_cmdBuff->makeRenderEncoder());
                           //_renderEncoder->beginRenderPass();
                       },
                       [&](RenderQueueData& renderQueue) {

                       },
                       [&](auto _) {

                       },
                   },
                   g[v].data);
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
    }

    rhi::CommandBufferPtr _cmdBuff;
    rhi::BlitEncoderPtr _blitEncoder;
    rhi::RenderEncoderPtr _renderEncoder;
    rhi::ComputeEncoderPtr _computeEncoder;
};

GraphScheduler::GraphScheduler(rhi::DevicePtr device): _device(device) {
    _renderGraph = new RenderGraph();
    _resourceGraph = new ResourceGraph(_device.get());
    _shaderGraph = new ShaderGraph(_device.get());
    _accessGraph = new AccessGraph(*_renderGraph, *_resourceGraph, *_shaderGraph);
    _sceneGraph = new SceneGraph();
    _taskGraph = new TaskGraph();
}

void GraphScheduler::execute() {
}

} // namespace raum::graph