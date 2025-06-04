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
#include "PBRMaterial.h"

namespace raum::graph {

namespace {

void prepareBindings(
    std::string_view phaseName,
    scene::TechniquePtr technique,
    scene::MeshRendererPtr meshrenderer,
    rhi::CompareOp zCmpOp,
    scene::SlotMap& perPassBindings,
    rhi::DescriptorSetLayoutInfo& perPassLayoutInfo,
    ShaderGraph& shg,
    rhi::DevicePtr device) {
    scene::SlotMap perBatchBindings;
    technique->depthStencilInfo().depthCompareOp = zCmpOp;
    if (phaseName == technique->phaseName()) {
        scene::SlotMap perInstanceBindings;
        const auto& shaderResource = shg.layout(technique->material()->shaderName());
        std::for_each(
            shaderResource.bindings.begin(),
            shaderResource.bindings.end(),
            [&perBatchBindings, &perInstanceBindings, &perPassBindings](const auto& p) {
                if (p.second.rate == Rate::PER_BATCH) {
                    perBatchBindings.emplace(p.first, p.second.binding);
                } else if (p.second.rate == Rate::PER_PASS) {
                    perPassBindings.emplace(p.first, p.second.binding);
                } else if (p.second.rate == Rate::PER_INSTANCE) {
                    perInstanceBindings.emplace(p.first, p.second.binding);
                }
            });

        if (meshrenderer) {
            meshrenderer->prepare(perInstanceBindings,
                                  shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_INSTANCE)],
                                  device);
        }

        if (!perBatchBindings.empty()) {
            technique->bakeMaterial(perBatchBindings,
                                    shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_BATCH)],
                                    device);
        }

        auto perPassLayout = shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_PASS)];
        for (const auto& binding : perPassLayout->info().descriptorBindings) {
            auto iter = std::find_if(perPassLayoutInfo.descriptorBindings.begin(),
                                     perPassLayoutInfo.descriptorBindings.end(),
                                     [&binding](const rhi::DescriptorBinding& bd) {
                                         return bd.binding == binding.binding;
                                     });
            if (iter == perPassLayoutInfo.descriptorBindings.end()) {
                perPassLayoutInfo.descriptorBindings.emplace_back(binding);
            }
        }
    }
}

} // namespace

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
            auto zCmpOp = test(queueData.flags, RenderQueueFlags::REVERSE_Z) ? rhi::CompareOp::GREATER_OR_EQUAL : rhi::CompareOp::LESS_OR_EQUAL;
            if (test(queueData.flags, RenderQueueFlags::GEOMETRY)) {
                for (auto& renderable : _rendererables) {
                    auto meshrenderer = std::static_pointer_cast<scene::MeshRenderer>(renderable);
                    // TODO: this is configed in editor
                    {
                        auto& techs = meshrenderer->techniques();
                        auto it = std::find_if(techs.begin(), techs.end(), [&phaseName](const auto& tech) {
                            return tech->phaseName() == phaseName;
                        });
                        if (it == techs.end()) {
                            auto embed = scene::EmbededTechniqueName.at(phaseName);
                            meshrenderer->addTechnique(scene::makeEmbededTechnique(embed));
                        }
                    }
                    for (auto& tech : meshrenderer->techniques()) {
                        prepareBindings(phaseName, tech, meshrenderer, zCmpOp, _perPassBindings, _perPassLayoutInfo, _shg, _device);
                    }
                }
            } else {
                // render screen quad
                prepareBindings(phaseName, queueData.technique, nullptr, zCmpOp, _perPassBindings, _perPassLayoutInfo, _shg, _device);
            }
        }
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
        if (std::holds_alternative<RenderPassData>(g[v].data)) {
            _perPassBindings.clear();
        } else if (std::holds_alternative<RenderQueueData>(g[v].data)) {
            auto& queueData = std::get<RenderQueueData>(_g.impl()[v].data);
            // auto& bindings = _perPassLayoutInfo.descriptorBindings;

            auto descLayout = rhi::getOrCreateDescriptorSetLayout(_perPassLayoutInfo, _device);
            queueData.bindGroup = std::make_shared<scene::BindGroup>(
                _perPassBindings,
                descLayout,
                _device);
            _perPhaseBindGroups.emplace(g[v].name, queueData.bindGroup);

            const auto& phaseName = getPhaseName(_g.impl()[v].name);
            if (test(queueData.flags, RenderQueueFlags::GEOMETRY)) {
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
            } else {
                auto& technique = queueData.technique;
                const auto& shaderResource = _shg.layout(technique->material()->shaderName());
                technique->bakePipeline(
                    _renderpass,
                    descLayout,
                    shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_BATCH)],
                    shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_INSTANCE)],
                    shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_DRAW)],
                    shaderResource.constants,
                    {},
                    shaderResource.shaderSources,
                    _device);
            }

        } else if (std::holds_alternative<ComputePassData>(g[v].data)) {
            auto& computeData = std::get<ComputePassData>(_g.impl()[v].data);
            const auto& programName = computeData.programName;
            const auto& shaderResource = _shg.layout(programName);
            scene::SlotMap perPassBindings;
            scene::SlotMap perBatchBindings;
            std::for_each(
                shaderResource.bindings.begin(),
                shaderResource.bindings.end(),
                [&perBatchBindings, &perPassBindings](const auto& p) {
                    if (p.second.rate == Rate::PER_PASS) {
                        perPassBindings.emplace(p.first, p.second.binding);
                    } else if (p.second.rate == Rate::PER_BATCH) {
                        perBatchBindings.emplace(p.first, p.second.binding);
                    }
                });

            auto method = scene::Method::pool().makeMethod(programName, flat_set<std::string>{});
            method->bakeBindGroup(
                perPassBindings,
                perBatchBindings,
                shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_PASS)],
                shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_BATCH)],
                _device);

            method->bakePipeline(
                shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_PASS)],
                shaderResource.descriptorLayouts[static_cast<uint32_t>(Rate::PER_BATCH)],
                shaderResource.constants,
                shaderResource.shaderSources,
                _device);

            computeData.method = method;
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
                                                        _resg.getImageView(renderingResource.name),
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
        } else if (std::holds_alternative<ComputePassData>(g[v].data)) {
            auto& compute = std::get<ComputePassData>(_g.impl()[v].data);
            compute.method = scene::Method::pool().makeMethod(compute.programName, flat_set<std::string>{});
            auto& method = compute.method;
            for (const auto& res : compute.resources) {
                _resg.mount(res.name);
                std::visit(overloaded{
                               [&](const BufferData& buffer) {
                                   method->bindBuffer(res.bindingName, 0, buffer.buffer);
                               },
                               [&](const BufferViewData& bufferView) {
                                   method->bindBuffer(res.bindingName,
                                                      0,
                                                      bufferView.info.offset,
                                                      bufferView.info.size,
                                                      _resg.getBuffer(bufferView.origin));
                               },
                               [&](const ImageData& image) {
                                   method->bindImage(res.bindingName,
                                                     0,
                                                     _resg.getImageView(res.name),
                                                     _ag.getImageLayout(res.name, v));
                               },
                               [&](const ImageViewData& imageView) {
                                   method->bindImage(res.bindingName,
                                                     0,
                                                     imageView.imageView,
                                                     _ag.getImageLayout(res.name, v));
                               },
                               [&](const SamplerData& sampler) {
                                   method->bindSampler(res.bindingName, 0, sampler.info);
                               },
                               [&](const rhi::SwapchainPtr& swapchain) {
                                   method->bindImage(res.bindingName,
                                                     0,
                                                     _resg.getImageView(res.name),
                                                     _ag.getImageLayout(res.name, v));
                               },
                               [](const auto&) {}},
                           _resg.get(res.name).data);
            }
            method->update();
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
                           if (test(data.flags, RenderQueueFlags::GEOMETRY)) {
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
                                   const auto& mat = technique->material();
                                   if (mat->type() == scene::MaterialType::PBR) {
                                       const auto& pbrMat = static_pointer_cast<scene::PBRMaterial>(mat);
                                       float alphCutoff = pbrMat->alphaCutoff();
                                       _renderEncoder->pushConstants(ShaderStage::FRAGMENT, 0, &alphCutoff, sizeof(float));
                                   }
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
                           } else {
                               const auto& quadTech = data.technique;
                               if (phase != quadTech->phaseName()) {
                                   return;
                               }
                               _renderEncoder->bindPipeline(quadTech->pipelineState().get());
                               if (quadTech->hasPassBinding()) [[likely]] {
                                   _renderEncoder->bindDescriptorSet(data.bindGroup->descriptorSet().get(), 0, nullptr, 0);
                               }
                               if (quadTech->hasBatchBinding()) {
                                   _renderEncoder->bindDescriptorSet(quadTech->material()->bindGroup()->descriptorSet().get(),
                                                                     1, nullptr, 0);
                               }
                               if (quadTech->hasInstanceBinding()) {
                                   // _renderEncoder->bindDescriptorSet(quadTech->bindGroup()->descriptorSet().get(), 2, nullptr, 0);
                               }

                               _renderEncoder->draw(3, 1, 0, 0);
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
                       [&](const ComputePassData& data) {
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

                           _computeEncoder = std::shared_ptr<rhi::RHIComputeEncoder>(_commandBuffer->makeComputeEncoder());
                           _computeEncoder->bindPipeline(data.method->pipelineState().get());
                           const auto& method = data.method;
                           if (method->hasPassBinding()) {
                               const auto& bindGroup = method->perPassBindGroup();
                               _computeEncoder->bindDescriptorSet(bindGroup->descriptorSet().get(), 0, nullptr, 0);
                           }
                           if (method->hasBatchBinding()) {
                               const auto& bindGroup = method->perBatchBindGroup();
                               _computeEncoder->bindDescriptorSet(bindGroup->descriptorSet().get(), 1, nullptr, 0);
                           }
                           const auto& dispatch = data.dispatch;
                           _computeEncoder->dispatch(dispatch.x, dispatch.y, dispatch.z);
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
                       [&](const ComputePassData&) {
                           _computeEncoder.reset();
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
    _accessGraph->analyze();

    if (!_warmed) {
        collectRenderables(_renderables, _cullableRenderables, _noCullRenderables, *_sceneGraph);

        _warmed = true;
        WarmUpVisitor warmUpVisitor{
            {},
            *_renderGraph,
            *_sceneGraph,
            *_accessGraph,
            *_shaderGraph,
            *_resourceGraph,
            _device,
            _renderables,
            _perPhaseBindGroups};

        visitRenderGraph(warmUpVisitor, *_renderGraph);

        _bvhRoot = buildBVH(_cullableRenderables, 1);
    }

    BVHCulling(_sceneGraph->cameras(), _bvhRoot, renderables);

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