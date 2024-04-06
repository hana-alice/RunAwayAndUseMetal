#include "GraphUtils.h"
#include "RHIDevice.h"

namespace raum::graph {

namespace {
static std::unordered_map<rhi::RenderPassInfo, rhi::RenderPassPtr, rhi::RHIHash<rhi::RenderPassInfo>> _renderPassMap;
static std::unordered_map<rhi::FrameBufferInfo, rhi::FrameBufferPtr, rhi::RHIHash<rhi::FrameBufferInfo>> _frameBufferMap;
static std::unordered_map<size_t, rhi::GraphicsPipelinePtr> _psoMap;
static std::unordered_map<size_t, rhi::DescriptorSetLayoutPtr> _descLayoutMap;
static std::unordered_map<size_t, rhi::PipelineLayoutPtr> _pplLayoutMap;
} // namespace

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

rhi::GraphicsPipelinePtr getOrCreateGraphicsPipeline(rhi::RenderPassPtr renderPass,
                                                     scene::MaterialPtr material,
                                                     scene::PhasePtr phase,
                                                     ShaderGraph& shg) {
    size_t seed = rhi::RHIHash<rhi::RenderPassInfo>{}(renderPass->info());
    boost::hash_combine(seed, material->shaderName());
    boost::hash_combine(seed, phase->phaseName());

    if (!_psoMap.contains(seed)) {
        rhi::GraphicsPipelineInfo info;
        info.primitiveType = phase->primitiveType();
        info.subpassIndex = 0;
        info.viewportCount = 1;
        info.rasterizationInfo = phase->rasterizationInfo();
        info.multisamplingInfo = phase->multisamplingInfo();
        info.depthStencilInfo = phase->depthStencilInfo();
        info.colorBlendInfo = phase->blendInfo();

        info.renderPass = renderPass.get();
        const auto& layout = shg.layout(material->shaderName());
        for (auto [_, shaderPtr] : layout.shaders) {
            info.shaders.emplace_back(shaderPtr.get());
        }
    }
    return _psoMap.at(seed);
}

rhi::DescriptorSetLayoutPtr getOrCreateDescriptorSetLayout(const rhi::DescriptorSetLayoutInfo& info, rhi::DevicePtr device) {
    size_t seed = rhi::RHIHash<rhi::DescriptorSetLayoutInfo>{}(info);
    if (!_descLayoutMap.contains(seed)) {
        _descLayoutMap[seed] = rhi::DescriptorSetLayoutPtr(device->createDescriptorSetLayout(info));
    }
    return _descLayoutMap.at(seed);
}

rhi::PipelineLayoutPtr getOrCreatePipelineLayout(const rhi::PipelineLayoutInfo& info, rhi::DevicePtr device) {
    size_t seed = 9527;
    for (auto* descLayout : info.setLayouts) {
        boost::hash_combine(seed, descLayout);
    }
    for (const auto& constantRange : info.pushConstantRanges) {
        boost::hash_combine(seed, constantRange.stage);
        boost::hash_combine(seed, constantRange.offset);
        boost::hash_combine(seed, constantRange.size);
    }
    if (!_pplLayoutMap.contains(seed)) {
        _pplLayoutMap[seed] = rhi::PipelineLayoutPtr(device->createPipelineLayout(info));
    }
    return _pplLayoutMap.at(seed);
}

}