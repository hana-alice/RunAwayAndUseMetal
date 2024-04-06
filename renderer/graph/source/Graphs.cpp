#include "Graphs.h"
#include <boost/graph/depth_first_search.hpp>
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIComputeEncoder.h"
#include "RHIDevice.h"
#include "RHIRenderEncoder.h"
#include "Mesh.h"
#include "Phase.h"
#include "GraphUtils.h"

namespace raum::graph {

bool culling(const ModelNode& node) {
    return true;
}

void collectRenderables(std::vector<scene::RenderablePtr> renderables, const SceneGraph& sg) {
    const auto& graph = sg.impl();
    for (auto v : boost::make_iterator_range(boost::vertices(graph))) {
        if (std::holds_alternative<ModelNode>(graph[v].sceneNodeData)) {
            const auto& modelNode = std::get<ModelNode>(graph[v].sceneNodeData);
            if (culling(modelNode)) {
                for (auto& mesh : modelNode.model->meshes()) {
                    renderables.emplace_back(mesh);
                }
            }
        }
    }
}

struct PreProcessVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](RenderPassData& renderpass) {
                           renderpass.renderpass = getOrCreateRenderPass(v, _ag, _device);
                           renderpass.framebuffer = getOrCreateFrameBuffer(renderpass.renderpass, v, _ag, _device);
                           renderpass.renderArea = {0,
                                                    0,
                                                    renderpass.framebuffer->info().width,
                                                    renderpass.framebuffer->info().height};
                           _perPassLayout.descriptorBindings.clear();
                       },
                       [&](RenderQueueData& queueData) {
                           auto phase = scene::getOrCreatePhase(queueData.phase);
                           //generateDescriptorSetLayout(,)
//                           for(const auto& layoutInfo : ) {
//
//                           }
                       },
                       [](auto _) {

                       },
                   },
                   g[v].data);
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
    }

    ResourceGraph& _resg;
    AccessGraph& _ag;
    ShaderGraph& _shg;
    SceneGraph& _sg;
    rhi::DevicePtr _device;
    rhi::DescriptorSetLayoutInfo _perPassLayout;
    std::array<rhi::DescriptorSetLayoutPtr, BindingRateCount> descriptorSetLayouts;
    std::vector<scene::Renderable>& _renderables;
};

struct RenderGraphVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](RenderPassData& data) {
                           _renderEncoder = std::shared_ptr<rhi::RHIRenderEncoder>(_cmdBuff->makeRenderEncoder());

                           std::vector<ClearValue> clears;
                           clears.reserve(data.attachments.size());
                           for (auto& am : data.attachments) {
                               clears.emplace_back(am.clearValue);
                           }

                           rhi::RenderPassBeginInfo beginInfo{
                               .renderPass = data.renderpass.get(),
                               .frameBuffer = data.framebuffer.get(),
                               .renderArea = data.renderArea,
                               .clearColors = clears.data(),
                           };

                           _renderEncoder->beginRenderPass(beginInfo);
                       },
                       [&](RenderQueueData& data) {
                           _renderEncoder->setViewport(data.viewport);

                           
                       },
                       [&](auto _) {

                       },
                   },
                   g[v].data);
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](RenderPassData&) {
                           _renderEncoder->endRenderPass();
                           _renderEncoder.reset();
                       },
                       [&](RenderQueueData& renderQueue) {

                       },
                       [&](auto _) {

                       },
                   },
                   g[v].data);
    }

    rhi::CommandBufferPtr _cmdBuff;
    rhi::BlitEncoderPtr _blitEncoder;
    rhi::RenderEncoderPtr _renderEncoder;
    rhi::ComputeEncoderPtr _computeEncoder;
};

GraphScheduler::GraphScheduler(rhi::DevicePtr device) : _device(device) {
    _renderGraph = new RenderGraph();
    _resourceGraph = new ResourceGraph(_device.get());
    _shaderGraph = new ShaderGraph(_device);
    _accessGraph = new AccessGraph(*_renderGraph, *_resourceGraph, *_shaderGraph);
    _sceneGraph = new SceneGraph();
    _taskGraph = new TaskGraph();
}

void GraphScheduler::execute() {
//    collectRenderables(queueData.renderables, _sg);
}

} // namespace raum::graph