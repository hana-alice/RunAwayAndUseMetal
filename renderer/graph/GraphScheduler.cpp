#include "GraphScheduler.h"
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
        if (std::holds_alternative<RenderPassData>(g[v].data)) {
            auto& renderpass = std::get<RenderPassData>(_g.impl()[v].data);
            renderpass.renderpass = getOrCreateRenderPass(v, _ag, _device);
            renderpass.renderArea = {0,
                                     0,
                                     _ag.getFrameBufferInfo(v)->info.width,
                                     _ag.getFrameBufferInfo(v)->info.height};
            _renderpass = renderpass.renderpass;
        } else if (std::holds_alternative<RenderQueueData>(g[v].data)) {
            const auto& phaseName = getPhaseName(_g.impl()[v].name);
            auto& queueData = std::get<RenderQueueData>(_g.impl()[v].data);
            scene::SlotMap perBatchBindings;
            for (auto& renderable : _rendererables) {
                auto meshrenderer = std::static_pointer_cast<scene::MeshRenderer>(renderable);
                for (auto& technique : meshrenderer->techniques()) {
                    if (phaseName == technique->phaseName()) {
                        scene::SlotMap perInstanceBindings;
                        const auto& shaderResource = _shg.layout(technique->material()->shaderName());
                        std::for_each(
                            shaderResource.bindings.begin(),
                            shaderResource.bindings.end(),
                            [&perBatchBindings, &perInstanceBindings, this](const auto& p) {
                                if (p.second.rate == Rate::PER_BATCH) {
                                    perBatchBindings.emplace(p.first, p.second.binding);
                                } else if (p.second.rate == Rate::PER_PASS) {
                                    _perPassBindings.emplace(p.first, p.second.binding);
                                } else if (p.second.rate == Rate::PER_INSTANCE) {
                                    perInstanceBindings.emplace(p.first, p.second.binding);
                                }
                            });

                        meshrenderer->prepare(perInstanceBindings,
                                              shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_INSTANCE)],
                                              _device);

                        technique->bakeMaterial(perBatchBindings,
                                                shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_BATCH)],
                                                _device);

                        auto perPassLayout = shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_PASS)];
                        for (const auto& binding : perPassLayout->info().descriptorBindings) {
                            auto iter = std::find_if(_perPassLayoutInfo.descriptorBindings.begin(),
                                                     _perPassLayoutInfo.descriptorBindings.end(),
                                                     [&binding](const rhi::DescriptorBinding& bd) {
                                                         return bd.binding == binding.binding;
                                                     });
                            if (iter == _perPassLayoutInfo.descriptorBindings.end()) {
                                _perPassLayoutInfo.descriptorBindings.emplace_back(binding);
                            }
                        }
                    }
                }
            }
        }
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        if (std::holds_alternative<RenderQueueData>(g[v].data)) {
            auto& queueData = std::get<RenderQueueData>(_g.impl()[v].data);
            // auto& bindings = _perPassLayoutInfo.descriptorBindings;

            auto descLayout = rhi::getOrCreateDescriptorSetLayout(_perPassLayoutInfo, _device);
            queueData.bindGroup = std::make_shared<scene::BindGroup>(
                _perPassBindings,
                descLayout,
                _device);
            _perPhaseBindGroups.emplace(g[v].name, queueData.bindGroup);

            const auto& phaseName = getPhaseName(_g.impl()[v].name);
            for (auto& renderable : _rendererables) {
                auto meshrenderer = std::static_pointer_cast<scene::MeshRenderer>(renderable);
                for (auto& technique : meshrenderer->techniques()) {
                    if (phaseName == technique->phaseName()) {
                        const auto& shaderResource = _shg.layout(technique->material()->shaderName());
                        technique->bakePipeline(
                            _renderpass,
                            descLayout,
                            shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_BATCH)],
                            shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_INSTANCE)],
                            shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_DRAW)],
                            shaderResource.constants,
                            meshrenderer->mesh()->meshData().vertexLayout,
                            shaderResource.shaderSources,
                            _device);
                    }
                }
            }
        }
    }

    RenderGraph& _g;
    SceneGraph& _sg;
    AccessGraph& _ag;
    ShaderGraph& _shg;
    ResourceGraph& _resg;
    rhi::DevicePtr _device;
    std::vector<scene::RenderablePtr>& _rendererables;
    std::unordered_map<std::string, scene::BindGroupPtr, hash_string, std::equal_to<>>& _perPhaseBindGroups;
    rhi::RenderPassPtr _renderpass;
    rhi::DescriptorSetLayoutInfo _perPassLayoutInfo;
    scene::SlotMap _perPassBindings;
};

struct PreProcessVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        if (std::holds_alternative<RenderPassData>(g[v].data)) {
            auto& renderpass = std::get<RenderPassData>(_g.impl()[v].data);
            for (auto attachment : renderpass.attachments) {
                _resg.mount(attachment.name);
            }
            renderpass.renderpass = getOrCreateRenderPass(v, _ag, _device);
            renderpass.renderArea = {0,
                                     0,
                                     _ag.getFrameBufferInfo(v)->info.width,
                                     _ag.getFrameBufferInfo(v)->info.height};
            renderpass.framebuffer = getOrCreateFrameBuffer(renderpass.renderpass, v, _ag, _resg, _device, _swapchain);
        } else if (std::holds_alternative<RenderQueueData>(g[v].data)) {
            auto& queueData = std::get<RenderQueueData>(_g.impl()[v].data);
            queueData.bindGroup = _perPhaseBindGroups.find(g[v].name)->second;
            auto bindGroup = queueData.bindGroup;
            for (auto renderingResource : queueData.resources) {
                _resg.mount(renderingResource.name);

                std::visit(overloaded{
                               [&](const BufferData& buffer) {
                                   bindGroup->bindBuffer(renderingResource.bindingName, 0, buffer.buffer);
                               },
                               [&](const BufferViewData& bufferView) {
                                   bindGroup->bindBuffer(renderingResource.bindingName,
                                                         0,
                                                         bufferView.info.offset,
                                                         bufferView.info.size,
                                                         _resg.getBuffer(bufferView.origin));
                               },
                               [&](const ImageData& image) {
                                   bindGroup->bindImage(renderingResource.bindingName,
                                                        0,
                                                        _resg.getImageView(renderingResource.bindingName),
                                                        _ag.getImageLayout(renderingResource.name, v));
                               },
                               [&](const ImageViewData& imageView) {
                                   bindGroup->bindImage(renderingResource.bindingName,
                                                        0,
                                                        imageView.imageView,
                                                        _ag.getImageLayout(renderingResource.name, v));
                               },
                               [&](const SamplerData& sampler) {
                                   bindGroup->bindSampler(renderingResource.bindingName, 0, sampler.info);
                               },
                               [&](const rhi::SwapchainPtr& swapchain) {
                                   bindGroup->bindImage(renderingResource.bindingName,
                                                        0,
                                                        _resg.getImageView(renderingResource.name),
                                                        _ag.getImageLayout(renderingResource.name, v));
                               },
                               [](auto _) {
                               }},
                           _resg.get(renderingResource.name).data);
            }
            queueData.bindGroup->update();

            for (auto& renderable : _renderables) {
                auto meshRenderer = std::static_pointer_cast<scene::MeshRenderer>(renderable);
                meshRenderer->update(_commandBuffer);
            }
        } else if (std::holds_alternative<CopyPassData>(g[v].data)) {
            auto& copy = std::get<CopyPassData>(_g.impl()[v].data);
            for (const auto& copyPair : copy.copies) {
                _resg.mount(copyPair.source);
                _resg.mount(copyPair.target);
            }
            for (const auto& upload : copy.uploads) {
                _resg.mount(upload.name);
            }
            for (const auto& fill : copy.fills) {
                _resg.mount(fill.name);
            }
        }
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
    }

    RenderGraph& _g;
    ResourceGraph& _resg;
    AccessGraph& _ag;
    ShaderGraph& _shg;
    SceneGraph& _sg;
    rhi::CommandBufferPtr _commandBuffer;
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;
    std::vector<scene::RenderablePtr>& _renderables;
    std::unordered_map<std::string, scene::BindGroupPtr, hash_string, std::equal_to<>>& _perPhaseBindGroups;
};

struct RenderGraphVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](const RenderPassData& data) {
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
                                   imageBarrier.info.image = _resg.getImage(imageBarrier.name).get();
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
                       [&](const RenderQueueData& data) {
                           std::string_view phase = getPhaseName(g[v].name);
                           _renderEncoder->setViewport(data.viewport);
                           _renderEncoder->setScissor(data.viewport.rect);
                           for (const auto& renderable : _renderables) {
                               const auto& meshRenderer = std::static_pointer_cast<scene::MeshRenderer>(renderable);
                               uint32_t phaseIndex = -1;
                               for (const auto& tech : meshRenderer->techniques()) {
                                   if (tech->phaseName() == phase) {
                                       phaseIndex = &tech - &meshRenderer->techniques()[0];
                                   }
                               }
                               raum_check(phaseIndex != -1, "Phase %s not found", phase);
                               const auto& technique = meshRenderer->technique(phaseIndex);
                               _renderEncoder->bindPipeline(technique->pipelineState().get());
                               if (technique->hasPassBinding()) {
                                   _renderEncoder->bindDescriptorSet(data.bindGroup->descriptorSet().get(), 0, nullptr, 0);
                               }
                               if (technique->hasBatchBinding()) [[likely]] {
                                   _renderEncoder->bindDescriptorSet(technique->material()->bindGroup()->descriptorSet().get(),
                                                                     1, nullptr, 0);
                               }
                               if (technique->hasInstanceBinding()) {
                                   _renderEncoder->bindDescriptorSet(meshRenderer->bindGroup()->descriptorSet().get(), 2, nullptr, 0);
                               }
                               const auto& drawInfo = meshRenderer->drawInfo();
                               const auto& meshData = meshRenderer->mesh()->meshData();
                               const auto& indexBuffer = meshData.indexBuffer;
                               const auto& vertexBuffer = meshData.vertexBuffer;
                               if (drawInfo.indexCount) {
                                   _renderEncoder->bindIndexBuffer(indexBuffer.buffer.get(), indexBuffer.offset, indexBuffer.type);
                                   _renderEncoder->bindVertexBuffer(vertexBuffer.buffer.get(), 0);
                                   _renderEncoder->drawIndexed(drawInfo.indexCount, drawInfo.instanceCount, drawInfo.firstVertex, drawInfo.vertexOffset, drawInfo.firstInstance);
                               } else {
                                   _renderEncoder->bindVertexBuffer(vertexBuffer.buffer.get(), 0);
                                   _renderEncoder->draw(drawInfo.vertexCount, drawInfo.instanceCount, drawInfo.firstVertex, drawInfo.firstInstance);
                               }
                           }
                       },
                       [&](const CopyPassData& copy) {
                           for (const auto& upload : copy.uploads) {
                               auto buffer = _resg.getBuffer(upload.name);
                               if (!_blitEncoder) {
                                   _blitEncoder = rhi::BlitEncoderPtr(_commandBuffer->makeBlitEncoder());
                               }
                               const auto& stagingBuffer = upload.stagingBuffer;
                               rhi::BufferCopyRegion region{
                                   .srcOffset = stagingBuffer.offset,
                                   .dstOffset = upload.offset,
                                   .size = upload.size,
                               };
                               _blitEncoder->copyBufferToBuffer(
                                   stagingBuffer.buffer.get(),
                                   buffer.get(),
                                   &region,
                                   1);
                           }
                           for (const auto& fill : copy.fills) {
                               auto buffer = _resg.getBuffer(fill.name);
                               if (!_blitEncoder) {
                                   _blitEncoder = rhi::BlitEncoderPtr(_commandBuffer->makeBlitEncoder());
                               }
                               _blitEncoder->fillBuffer(buffer.get(), fill.dstOffset, fill.size, fill.value);
                           }
                       },
                       [&](auto _) {

                       },
                   },
                   g[v].data);
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        std::visit(overloaded{
                       [&](const RenderPassData&) {
                           _renderEncoder->endRenderPass();
                           _renderEncoder.reset();
                       },
                       [&](const RenderQueueData& renderQueue) {

                       },
                       [&](const CopyPassData& renderQueue) {
                           _blitEncoder.reset();
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

GraphScheduler::GraphScheduler(
    rhi::DevicePtr device,
    rhi::SwapchainPtr swapchain,
    RenderGraph* renderGraph,
    ResourceGraph* resourceGraph,
    AccessGraph* accessGraph,
    TaskGraph* taskGraph,
    SceneGraph* sceneGraph,
    ShaderGraph* shaderGraph)
: _device(device),
  _swapchain(swapchain),
  _renderGraph(renderGraph),
  _resourceGraph(resourceGraph),
  _accessGraph(accessGraph),
  _taskGraph(taskGraph),
  _sceneGraph(sceneGraph),
  _shaderGraph(shaderGraph) {
}

template <typename T>
concept GraphVisitor = std::is_base_of_v<boost::dfs_visitor<>, T>;

template <GraphVisitor T>
void visitRenderGraph(T& visitor, RenderGraph& renderGraph) {
    auto& g = renderGraph.impl();
    auto indexMap = boost::get(boost::vertex_index, g);
    auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);

    for (auto vertex : boost::make_iterator_range(boost::vertices(g))) {
        if (!std::holds_alternative<RenderQueueData>(g[vertex].data)) {
            boost::depth_first_visit(g, vertex, visitor, colorMap);
        }
    }
}

void GraphScheduler::needWarmUp() {
    _warmed = false;
}

void GraphScheduler::execute(rhi::CommandBufferPtr cmd) {
    std::vector<scene::RenderablePtr> renderables;
    collectRenderables(renderables, *_sceneGraph, _warmed);

    _accessGraph->analyze();

    if (!_warmed) {
        _warmed = true;
        WarmUpVisitor warmUpVisitor{
            {},
            *_renderGraph,
            *_sceneGraph,
            *_accessGraph,
            *_shaderGraph,
            *_resourceGraph,
            _device,
            renderables,
            _perPhaseBindGroups};

        visitRenderGraph(warmUpVisitor, *_renderGraph);
    }

    PreProcessVisitor preProcessVisitor{
        {},
        *_renderGraph,
        *_resourceGraph,
        *_accessGraph,
        *_shaderGraph,
        *_sceneGraph,
        cmd,
        _device,
        _swapchain,
        renderables,
        _perPhaseBindGroups};
    visitRenderGraph(preProcessVisitor, *_renderGraph);

    RenderGraphVisitor encodeVisitor{{}, *_accessGraph, *_resourceGraph, renderables, cmd};
    visitRenderGraph(encodeVisitor, *_renderGraph);

    auto* presentBarrier = _accessGraph->presentBarrier();
    if (presentBarrier) {
        presentBarrier->info.image = _resourceGraph->getImage(presentBarrier->name).get();
        cmd->appendImageBarrier(presentBarrier->info);
        cmd->applyBarrier(rhi::DependencyFlags::BY_REGION);
    }

    _renderGraph->clear();
    _accessGraph->clear();
}

} // namespace raum::graph