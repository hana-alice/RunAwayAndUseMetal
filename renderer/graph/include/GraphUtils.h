#pragma once
#include "RHIDevice.h"
#include "RenderGraph.h"
#include "Material.h"
#include "ShaderGraph.h"
#include "AccessGraph.h"
namespace raum::graph {

	
rhi::RenderPassPtr getOrCreateRenderPass(const RenderGraph::VertexType v, AccessGraph& ag, rhi::DevicePtr device);

rhi::FrameBufferPtr getOrCreateFrameBuffer(rhi::RenderPassPtr renderpass, const RenderGraph::VertexType v, AccessGraph& ag, rhi::DevicePtr device);

rhi::GraphicsPipelinePtr getOrCreateGraphicsPipeline(rhi::RenderPassPtr renderPass,
                                                     scene::MaterialPtr material,
                                                     scene::PhasePtr phase,
                                                     ShaderGraph& shg);

rhi::DescriptorSetLayoutPtr getOrCreateDescriptorSetLayout(const rhi::DescriptorSetLayoutInfo& info, rhi::DevicePtr device);

rhi::PipelineLayoutPtr getOrCreatePipelineLayout(const rhi::PipelineLayoutInfo& info, rhi::DevicePtr device);

}
