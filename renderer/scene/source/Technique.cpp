#include "Technique.h"
#include "RHIDevice.h"
#include "RHIUtils.h"
#include <algorithm>
namespace raum::scene {

namespace {
std::unordered_map<rhi::GraphicsPipelineInfo, rhi::GraphicsPipelinePtr, rhi::RHIHash<rhi::GraphicsPipelineInfo>> _psoMap;
}

Technique::Technique(MaterialPtr material, std::string_view phaseName)
: _material(material), _phaseName(phaseName) {
}

MaterialPtr Technique::material() {
    return _material;
}

const std::string& Technique::phaseName() const {
    return _phaseName;
}

rhi::RasterizationInfo& Technique::rasterizationInfo() {
    return _rasterizationInfo;
}

rhi::DepthStencilInfo& Technique::depthStencilInfo() {
    return _depthStencilInfo;
}

rhi::MultisamplingInfo& Technique::multisamplingInfo() {
    return _multisamplingInfo;
}

rhi::BlendInfo& Technique::blendInfo() {
    return _blendInfo;
}

rhi::GraphicsPipelinePtr Technique::pipelineState() {
    return _pso;
}

void Technique::setPrimitiveType(rhi::PrimitiveType type) {
    _primitiveType = type;
}

void Technique::bake(rhi::RenderPassPtr renderpass,
                     rhi::PipelineLayoutPtr pplLayout,
                     rhi::VertexLayout vertexLayout,
                     const boost::container::flat_map<rhi::ShaderStage, rhi::ShaderPtr>& shaderIn,
                     const boost::container::flat_map<std::string_view, uint32_t>& perBatchBinding,
                     rhi::DescriptorSetLayoutPtr batchLayout,
                     rhi::DevicePtr device) {
    std::vector<rhi::RHIShader*> shaders;
    shaders.reserve(shaderIn.size());
    std::for_each(shaderIn.begin(), shaderIn.end(), [&shaders](const auto& p) {
        shaders.emplace_back(p.second.get());
    });

    const auto& attachments = renderpass->attachments();
    auto colorSize = std::count_if(attachments.begin(), attachments.end(), [](const rhi::AttachmentInfo& attachment) {
        return attachment.finalLayout == rhi::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
    });
    _blendInfo.attachmentBlends.resize(colorSize);

    rhi::GraphicsPipelineInfo info{
        .primitiveType = _primitiveType,
        .pipelineLayout = pplLayout.get(),
        .renderPass = renderpass.get(),
        .shaders = shaders,
        .subpassIndex = 0,
        .viewportCount = 1,
        .vertexLayout = vertexLayout,
        .rasterizationInfo = _rasterizationInfo,
        .multisamplingInfo = _multisamplingInfo,
        .depthStencilInfo = _depthStencilInfo,
        .colorBlendInfo = _blendInfo,
    };
    if(!_psoMap.contains(info)) {
        _psoMap.emplace(info, rhi::GraphicsPipelinePtr (device->createGraphicsPipeline(info)));
    }
    _pso = _psoMap.at(info);

    _material->initBindGroup(perBatchBinding, batchLayout, device);
}



} // namespace raum::scene
