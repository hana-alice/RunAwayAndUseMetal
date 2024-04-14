#include "Graphs.h"
#include <boost/graph/depth_first_search.hpp>
#include "GraphUtils.h"
#include "Mesh.h"
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHIComputeEncoder.h"
#include "RHIDevice.h"
#include "RHIRenderEncoder.h"
#include "RHIUtils.h"

namespace raum::graph {

struct WarmUpVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](RenderPassData& renderpass) {
                           renderpass.renderpass = getOrCreateRenderPass(v, _ag, _device);
                           renderpass.renderArea = {0,
                                                    0,
                                                    _ag.getFrameBufferInfo(v)->info.width,
                                                    _ag.getFrameBufferInfo(v)->info.height};
                           _renderpass = renderpass.renderpass;
                       },
                       [&](RenderQueueData& queueData) {
                           boost::container::flat_map<std::string_view, uint32_t> perBatchBindings;
                           for (auto& desc : queueData.resources) {
                               for (auto& renderable : _rendererables) {
                                   auto meshrenderer = std::static_pointer_cast<scene::MeshRenderer>(renderable);
                                   for (auto& technique : meshrenderer->techniques()) {
                                       if (queueData.phase == technique->phaseName()) {
                                           const auto& shaderResource = _shg.layout(technique->material()->shaderName());
                                           std::for_each(
                                               shaderResource.bindings.begin(),
                                               shaderResource.bindings.end(),
                                               [&perBatchBindings, this](const auto& p) {
                                                   if (p.second.rate == Rate::PER_BATCH) {
                                                       perBatchBindings.emplace(p.first, p.second.binding);
                                                   } else if (p.second.rate == Rate::PER_PASS) {
                                                       _perPassBindings.emplace(p.first, p.second.binding);
                                                   }
                                               });

                                           technique->bake(_renderpass,
                                                           shaderResource.pipelineLayout,
                                                           meshrenderer->mesh()->meshData().vertexLayout,
                                                           shaderResource.shaders,
                                                           perBatchBindings,
                                                           shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_BATCH)],
                                                           _device);

                                           auto perPassLayout = shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_PASS)];
                                           _perPassLayoutInfo.descriptorBindings.insert(
                                               _perPassLayoutInfo.descriptorBindings.end(),
                                               perPassLayout->info().descriptorBindings.begin(),
                                               perPassLayout->info().descriptorBindings.end());
                                       }
                                   }
                               }
                           }
                       },
                       [](auto _) {
                       },
                   },
                   g[v].data);
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](RenderPassData& renderpass) {
                       },
                       [&](RenderQueueData& queueData) {
                           auto& bindings = _perPassLayoutInfo.descriptorBindings;
                           std::sort(bindings.begin(), bindings.end(),
                                     [](const rhi::DescriptorBinding& lhs, const rhi::DescriptorBinding& rhs) {
                                         return lhs.binding < rhs.binding;
                                     });
                           bindings.erase(std::unique(bindings.begin(),
                                                      bindings.end(),
                                                      [](const rhi::DescriptorBinding& lhs, const rhi::DescriptorBinding& rhs) {
                                                          return lhs.binding < rhs.binding;
                                                      }),
                                          bindings.end());
                           auto descLayout = rhi::getOrCreateDescriptorSetLayout(_perPassLayoutInfo, _device);
                           queueData.bindGroup = std::make_shared<scene::BindGroup>(
                               _perPassBindings,
                               descLayout,
                               _device);
                       },
                       [](auto _) {
                       },
                   },
                   g[v].data);
    }

    SceneGraph& _sg;
    AccessGraph& _ag;
    ShaderGraph& _shg;
    ResourceGraph& _resg;
    rhi::DevicePtr _device;
    std::vector<scene::RenderablePtr>& _rendererables;
    rhi::RenderPassPtr _renderpass;
    rhi::DescriptorSetLayoutInfo _perPassLayoutInfo;
    boost::container::flat_map<std::string_view, uint32_t> _perPassBindings;
};

struct PreProcessVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](RenderPassData& renderpass) {
                           for (auto attachment : renderpass.attachments) {
                               _resg.mount(attachment.name);
                           }
                           renderpass.framebuffer = getOrCreateFrameBuffer(renderpass.renderpass, v, _ag, _device);
                       },
                       [&](RenderQueueData& queueData) {
                           auto bindGroup = queueData.bindGroup;
                           for (auto renderingResource : queueData.resources) {
                               _resg.mount(renderingResource.name);

                               std::visit(overloaded{
                                              [&](const BufferData& buffer){
                                                  bindGroup->bindBuffer(renderingResource.bindingName, 0, buffer.buffer);
                                              },
                                              [&](const BufferViewData& bufferView) {
                                                bindGroup->bindBuffer(renderingResource.bindingName,
                                                                        0,
                                                                        bufferView.info.offset,
                                                                        bufferView.info.size,
                                                                        _resg.getBuffer(bufferView.origin));
                                              },
                                              [&](const ImageData& image){
                                                  bindGroup->bindImage(renderingResource.bindingName,
                                                                       0,
                                                                       _resg.getImageView(renderingResource.bindingName),
                                                                       _ag.getImageLayout(renderingResource.name, v));
                                              },
                                              [&](const ImageViewData& imageView){
                                                  bindGroup->bindImage(renderingResource.bindingName,
                                                                       0,
                                                                       imageView.imageView,
                                                                       _ag.getImageLayout(renderingResource.name, v));
                                              },
                                              [&](const rhi::SwapchainPtr& swapchain){
//                                                  bindGroup->bindImage(renderingResource.bindingName,
//                                                                       0,
//                                                                       imageView.imageView,
//                                                                       _ag.getImageLayout(renderingResource.name, v));
                                              },
                                              [](auto _) {
                                              }},
                                          _resg.get(renderingResource.name).data);
                           }
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
    rhi::CommandBufferPtr _commandBuffer;
    rhi::DevicePtr _device;
    std::vector<scene::RenderablePtr>& _renderables;
};

struct RenderGraphVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](RenderPassData& data) {
                           auto* bufferBarriers = _accessGraph.getBufferBarrier(v);
                           if (bufferBarriers) {
                               for (auto& bufferBarrier : *bufferBarriers) {
                                   bufferBarrier.info.buffer = std::get<BufferData>(_resg.get(bufferBarrier.name).data).buffer.get();
                                   _commandBuffer->appendBufferBarrier(bufferBarrier.info);
                               }
                           }

                           auto* imageBarriers = _accessGraph.getImageBarrier(v);
                           if (imageBarriers) {
                               for (auto& imageBarrier : *imageBarriers) {
                                   imageBarrier.info.image = std::get<ImageData>(_resg.get(imageBarrier.name).data).image.get();
                                   _commandBuffer->appendImageBarrier(imageBarrier.info);
                               }
                           }
                           _commandBuffer->applyBarrier(rhi::DependencyFlags::BY_REGION);

                           _renderEncoder = std::shared_ptr<rhi::RHIRenderEncoder>(_commandBuffer->makeRenderEncoder());
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
                           for (const auto& renderable : _renderables) {
                               const auto& meshRenderer = std::static_pointer_cast<scene::MeshRenderer>(renderable);
                               if (meshRenderer->technique(0)->phaseName() == data.phase) {
                                   _renderEncoder->bindDescriptorSet(data.bindGroup->descriptorSet().get(), 0, nullptr, 0);
                               }
                           }
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

    AccessGraph& _accessGraph;
    ResourceGraph& _resg;
    const std::vector<scene::RenderablePtr> _renderables;
    rhi::CommandBufferPtr _commandBuffer;
    rhi::BlitEncoderPtr _blitEncoder;
    rhi::RenderEncoderPtr _renderEncoder;
    rhi::ComputeEncoderPtr _computeEncoder;
};

GraphScheduler::GraphScheduler(rhi::DevicePtr device, rhi::SwapchainPtr swapchain) : _device(device), _swapchian(swapchain) {
    _renderGraph = new RenderGraph();
    _resourceGraph = new ResourceGraph(_device.get());
    _shaderGraph = new ShaderGraph(_device);
    _accessGraph = new AccessGraph(*_renderGraph, *_resourceGraph, *_shaderGraph);
    _sceneGraph = new SceneGraph();
    _taskGraph = new TaskGraph();
    _commandPool = rhi::CommandPoolPtr(device->createCoomandPool({device->getQueue({rhi::QueueType::GRAPHICS})->index()}));
}

template <typename T>
concept GraphVisitor = std::is_base_of_v<boost::dfs_visitor<>, T>;

template <GraphVisitor T>
void visitRenderGraph(T& visitor, RenderGraph& renderGraph) {
    auto& g = renderGraph.impl();
    auto indexMap = boost::get(boost::vertex_index, g);
    auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);

    for (auto vertex : boost::make_iterator_range(boost::vertices(g))) {
        if (std::holds_alternative<RenderPassData>(g[vertex].data)) {
            boost::depth_first_visit(g, 0, visitor, colorMap);
        }
    }
}

void GraphScheduler::execute() {
    static bool warmed{false};

    rhi::CommandBufferPtr cmdBuffer;
    if (!_commandBuffers.contains(_swapchian->swapchainImageView())) {
        _commandBuffers[_swapchian->swapchainImageView()] = rhi::CommandBufferPtr(_commandPool->makeCommandBuffer({}));
    }
    cmdBuffer = _commandBuffers[_swapchian->swapchainImageView()];

    std::vector<scene::RenderablePtr> renderables;
    collectRenderables(renderables, *_sceneGraph, warmed);
    _accessGraph->analyze();

    if (!warmed) {
        warmed = true;
        WarmUpVisitor warmUpVisitor{
            {},
            *_sceneGraph,
            *_accessGraph,
            *_shaderGraph,
            *_resourceGraph,
            _device,
            renderables};

        visitRenderGraph(warmUpVisitor, *_renderGraph);
    }

    PreProcessVisitor preProcessVisitor{
        {},
        *_resourceGraph,
        *_accessGraph,
        *_shaderGraph,
        *_sceneGraph,
        cmdBuffer,
        _device,
        renderables};
    visitRenderGraph(preProcessVisitor, *_renderGraph);
//
//    RenderGraphVisitor encodeVisitor{{}, *_accessGraph, *_resourceGraph, renderables, cmdBuffer};
//    visitRenderGraph(encodeVisitor, *_renderGraph);
//
//    auto* presentBarrier = _accessGraph->presentBarrier();
//    if (presentBarrier) {
//        presentBarrier->info.image = std::get<ImageData>(_resourceGraph->get(presentBarrier->name).data).image.get();
//        cmdBuffer->appendImageBarrier(presentBarrier->info);
//        cmdBuffer->applyBarrier(rhi::DependencyFlags::BY_REGION);
//    }
//    auto* queue = _device->getQueue({rhi::QueueType::GRAPHICS});
//    cmdBuffer->commit(queue);
//    queue->submit();
//    _swapchian->present();
//
//    _renderGraph->clear();
//    _accessGraph->clear();
}

} // namespace raum::graph