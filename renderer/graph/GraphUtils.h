#pragma once
#include <span>
#include "AccessGraph.h"
#include "Material.h"
#include "RHIDevice.h"
#include "RenderGraph.h"
#include "SceneGraph.h"
#include "ShaderGraph.h"
namespace raum::graph {

rhi::RenderPassPtr getOrCreateRenderPass(const RenderGraph::VertexType v, AccessGraph& ag, rhi::DevicePtr device);

rhi::FrameBufferPtr getOrCreateFrameBuffer(rhi::RenderPassPtr renderpass,
                                           const RenderGraph::VertexType v,
                                           AccessGraph& ag,
                                           ResourceGraph& resg,
                                           rhi::DevicePtr device,
                                           rhi::SwapchainPtr swapchain);

rhi::GraphicsPipelinePtr getOrCreateGraphicsPipeline(rhi::RenderPassPtr renderPass,
                                                     scene::MaterialPtr material,
                                                     ShaderGraph& shg);

bool culling(const ModelNode& node);

void collectRenderables(std::vector<scene::RenderablePtr>& renderables,
                        std::span<scene::RenderablePtr>& cullableRenderables,
                        std::span<scene::RenderablePtr>& nocullRenderables,
                        const SceneGraph& sg);

void BVHCulling(const std::vector<CameraNode*>& cameras,
                const scene::BVHNode* node,
                std::vector<scene::RenderablePtr>& renderables);

scene::BVHNode* buildBVH(std::span<scene::RenderablePtr>& renderables, uint32_t maxObjectsPerNode = 4);

void warmUp(SceneGraph& sg, ShaderGraph& shg, rhi::DevicePtr device);

std::string_view getPhaseName(std::string_view queueName);

void bindResourceToMaterial(std::string_view resourceName, std::string_view slotName, scene::MaterialPtr mat, ResourceGraph& resg);

scene::MeshRendererPtr getLocalQuad(rhi::DevicePtr device);

} // namespace raum::graph
