#include "Graphs.h"
#include <boost/graph/depth_first_search.hpp>
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIComputeEncoder.h"
#include "RHIRenderEncoder.h"
#include "RHIDevice.h"

namespace raum::graph {

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

void execute(RenderGraph& rg, ResourceGraph& resg, ShaderGraph& shg, SceneGraph& sg, TaskGraph& tg) {
    AccessGraph ag{rg, resg, shg};
    ag.analyze();
}

} // namespace raum::graph