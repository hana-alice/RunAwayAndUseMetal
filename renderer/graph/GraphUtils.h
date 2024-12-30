#pragma once
#include "RHIDevice.h"
#include "RenderGraph.h"
#include "Material.h"
#include "ShaderGraph.h"
#include "AccessGraph.h"
#include "SceneGraph.h"
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

void collectRenderables(std::vector<scene::RenderablePtr>& renderables, const SceneGraph& sg, bool enableCullling);

void warmUp(SceneGraph& sg, ShaderGraph& shg, rhi::DevicePtr device);

std::string_view getPhaseName(std::string_view queueName);

}
