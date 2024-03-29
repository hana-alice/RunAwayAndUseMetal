#include "Graphs.h"
#include <boost/graph/depth_first_search.hpp>
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIComputeEncoder.h"
#include "RHIDevice.h"
#include "RHIRenderEncoder.h"

namespace raum::graph {

namespace {
static std::unordered_map<rhi::RenderPassInfo, rhi::RenderPassPtr, rhi::RHIHash<rhi::RenderPassInfo>> _renderPassMap;
static std::unordered_map<rhi::FrameBufferInfo, rhi::FrameBufferPtr, rhi::RHIHash<rhi::FrameBufferInfo>> _frameBufferMap;

rhi::RenderPassPtr getOrCreateRenderPass(const RenderGraph::VertexType v, AccessGraph& ag, rhi::DevicePtr device) {
    auto* renderpassInfo = ag.getRenderPassInfo(v);
    raum_check(renderpassInfo, "failed to analyze renderpass info at {}", v);
    if (renderpassInfo) {
        if (!_renderPassMap.contains(*renderpassInfo)) {
            _renderPassMap[*renderpassInfo] = rhi::RenderPassPtr(device->createRenderPass(*renderpassInfo));
        }
        return _renderPassMap[*renderpassInfo];
    }
    raum_unreachable();
    return nullptr;
}

rhi::FrameBufferPtr getOrCreateFrameBuffer(rhi::RenderPassPtr renderpass, const RenderGraph::VertexType v, AccessGraph& ag, rhi::DevicePtr device) {
    auto* framebufferInfo = ag.getFrameBufferInfo(v);
    framebufferInfo->renderPass = renderpass.get();
    raum_check(framebufferInfo, "failed to analyze framebuffer info at {}", v);
    if (framebufferInfo) {
        if (!_frameBufferMap.contains(*framebufferInfo)) {
            _frameBufferMap[*framebufferInfo] = rhi::FrameBufferPtr(device->createFrameBuffer(*framebufferInfo));
        }
        return _frameBufferMap[*framebufferInfo];
    }
    raum_unreachable();
    return nullptr;
}

bool culling(const ModelNode& node) {
    return true;
}
} // namespace

void collectRenderables(std::map<uint32_t, scene::RenderablePtr>& renderables, const SceneGraph& sg) {
    const auto& graph = sg.impl();
    for (auto v : boost::make_iterator_range(boost::vertices(graph))) {
        if (std::holds_alternative<ModelNode>(graph[v].sceneNodeData)) {
            const auto& modelNode = std::get<ModelNode>(graph[v].sceneNodeData);
            if (culling(modelNode)) {
                for (auto& mesh : modelNode.model->meshes) {
                    renderables.emplace(mesh->materialID, mesh);
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

    ResourceGraph& _resg;
    AccessGraph& _ag;
    SceneGraph& _sg;
    rhi::DevicePtr _device;
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
    _shaderGraph = new ShaderGraph(_device.get());
    _accessGraph = new AccessGraph(*_renderGraph, *_resourceGraph, *_shaderGraph);
    _sceneGraph = new SceneGraph();
    _taskGraph = new TaskGraph();
}

void GraphScheduler::execute() {
}

} // namespace raum::graph