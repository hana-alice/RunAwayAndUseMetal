#include "Technique.h"
#include <algorithm>
#include "RHIDevice.h"
#include "RHIUtils.h"
#include <boost/functional/hash.hpp>
#include <set>
namespace raum::scene {

namespace {
std::unordered_map<rhi::GraphicsPipelineInfo, rhi::GraphicsPipelinePtr, rhi::RHIHash<rhi::GraphicsPipelineInfo>> _psoMap;
std::unordered_map<std::size_t, rhi::ShaderPtr> _shaderMap;
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
                     const boost::container::flat_map<rhi::ShaderStage, std::string>& shaderIn,
                     const boost::container::flat_map<std::string_view, uint32_t>& perBatchBinding,
                     rhi::DescriptorSetLayoutPtr batchLayout,
                     rhi::DevicePtr device) {
    if (_material->bindGroup()) {
        // shared material, already been initialized.
        return;
    }
    std::vector<rhi::RHIShader*> shaders;
    shaders.reserve(shaderIn.size());

    std::ranges::for_each(shaderIn, [device, &shaders, this](const auto& p) {
        auto shaderPath = _material->shaderName();
        size_t seed = 9527;
        boost::hash_combine(seed, shaderPath);
        std::string prefix = "#version 450 core\n";
        std::ranges::for_each(_material->defines(), [&seed, &prefix](const std::string& s) {
            boost::hash_combine(seed, s);
            prefix.append("#define " + s + '\n');
        });
        boost::hash_combine(seed, p.first);
        if (!_shaderMap.contains(seed)) {
            rhi::ShaderSourceInfo info{
                shaderPath.data(),
                {p.first, prefix + p.second},
            };
            _shaderMap.emplace(seed, rhi::ShaderPtr(device->createShader(info)));
        }
        auto shader = _shaderMap.at(seed);
        shaders.emplace_back(shader.get());
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
    if (!_psoMap.contains(info)) {
        _psoMap.emplace(info, rhi::GraphicsPipelinePtr(device->createGraphicsPipeline(info)));
    }
    _pso = _psoMap.at(info);

    _material->initBindGroup(perBatchBinding, batchLayout, device);
}

} // namespace raum::scene
