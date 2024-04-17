#pragma once
#include "RHIDevice.h"
#include "RenderGraph.h"
#include "Material.h"
#include "ShaderGraph.h"
#include "AccessGraph.h"
#include "SceneGraph.h"
namespace raum::graph {

	
rhi::RenderPassPtr getOrCreateRenderPass(const RenderGraph::VertexType v, AccessGraph& ag, rhi::DevicePtr device);

rhi::FrameBufferPtr getOrCreateFrameBuffer(rhi::RenderPassPtr renderpass, const RenderGraph::VertexType v, AccessGraph& ag, ResourceGraph& resg, rhi::DevicePtr device);

rhi::GraphicsPipelinePtr getOrCreateGraphicsPipeline(rhi::RenderPassPtr renderPass,
                                                     scene::MaterialPtr material,
                                                     ShaderGraph& shg);

bool culling(const ModelNode& node);

void collectRenderables(std::vector<scene::RenderablePtr>& renderables, const SceneGraph& sg, bool enableCullling);

void warmUp(SceneGraph& sg, ShaderGraph& shg, rhi::DevicePtr device);

void bakePipelineState(scene::MeshRendererPtr meshRenderer, rhi::RenderPassPtr renderPass, scene::TechniquePtr technique, rhi::DevicePtr device);

}
