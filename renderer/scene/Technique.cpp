#include "Technique.h"
#include <algorithm>
#include <boost/functional/hash.hpp>
#include <set>
#include "RHIDevice.h"
#include "RHIUtils.h"

namespace raum::scene {
namespace {
std::unordered_map<EmbededTechnique, scene::TechniquePtr> embededTechs;
std::unordered_map<rhi::GraphicsPipelineInfo, rhi::GraphicsPipelinePtr, rhi::RHIHash<rhi::GraphicsPipelineInfo>> _psoMap;
std::unordered_map<std::size_t, rhi::ShaderPtr> _shaderMap;
} // namespace

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

void Technique::bakeMaterial(const SlotMap& perBatchBinding,
                             rhi::DescriptorSetLayoutPtr batchLayout,
                             rhi::DevicePtr device) {
    if (_material->bindGroup()) {
        // shared material, already been initialized.
        return;
    }
    _material->initBindGroup(perBatchBinding, batchLayout, device);
    _material->update();
}

void Technique::bakePipeline(rhi::RenderPassPtr renderpass,
                             rhi::DescriptorSetLayoutPtr passDescSet,
                             rhi::DescriptorSetLayoutPtr batchDescSet,
                             rhi::DescriptorSetLayoutPtr instDescSet,
                             rhi::DescriptorSetLayoutPtr drawDescSet,
                             const std::vector<rhi::PushConstantRange>& constants,
                             rhi::VertexLayout vertexLayout,
                             const boost::container::flat_map<rhi::ShaderStage, std::string>& shaderIn,
                             rhi::DevicePtr device) {
    if (_pso) {
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
    std::vector<rhi::RHIDescriptorSetLayout*> setLayouts = {
        passDescSet.get(),
        batchDescSet.get(),
        instDescSet.get(),
        drawDescSet.get(),
    };

    _bindingBound = {
        passDescSet && !passDescSet->info().descriptorBindings.empty(),
        batchDescSet && !batchDescSet->info().descriptorBindings.empty(),
        instDescSet && !instDescSet->info().descriptorBindings.empty(),
        drawDescSet && !drawDescSet->info().descriptorBindings.empty(),
    };
    rhi::PipelineLayoutInfo layoutInfo = {
        constants,
        setLayouts,
    };
    auto pplLayout = rhi::getOrCreatePipelineLayout(layoutInfo, device);

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
}

bool Technique::hasPassBinding() const {
    return _bindingBound[0];
}

bool Technique::hasBatchBinding() const {
    return _bindingBound[1];
}

bool Technique::hasInstanceBinding() const {
    return _bindingBound[2];
}

bool Technique::hasDrawBinding() const {
    return _bindingBound[3];
}

template <EmbededTechnique T>
TechniquePtr makeTechnique();

template <>
TechniquePtr makeTechnique<EmbededTechnique::SHADOWMAP>() {
    scene::MaterialTemplatePtr shadowTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/shadowMap");
    auto shadowMapMaterial = shadowTemplate->instantiate("asset/layout/shadowMap", scene::MaterialType::CUSTOM);
    auto shadowTech = std::make_shared<scene::Technique>(shadowMapMaterial, "shadowMap");

    auto& ds = shadowTech->depthStencilInfo();
    ds.depthTestEnable = true;
    ds.depthWriteEnable = true;
    auto& bs = shadowTech->blendInfo();
    bs.attachmentBlends.emplace_back();
    return shadowTech;
}

template <>
TechniquePtr makeTechnique<EmbededTechnique::SOLID_COLOR>() {
    scene::MaterialTemplatePtr solidTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/solidColor");
    auto solidColorMaterial = solidTemplate->instantiate("asset/layout/solidColor", scene::MaterialType::CUSTOM);
    auto solidTech = std::make_shared<scene::Technique>(solidColorMaterial, "solidColor");

    auto& ds = solidTech->depthStencilInfo();
    ds.depthTestEnable = true;
    ds.depthWriteEnable = true;
    auto& bs = solidTech->blendInfo();
    bs.attachmentBlends.emplace_back();
    return solidTech;
}

TechniquePtr makeEmbededTechnique(EmbededTechnique type) {
    TechniquePtr tech;
    switch (type) {
        case EmbededTechnique::SHADOWMAP: {
            tech = makeTechnique<EmbededTechnique::SHADOWMAP>();
            break;
        }
        case EmbededTechnique::SOLID_COLOR: {
            tech = makeTechnique<EmbededTechnique::SOLID_COLOR>();
            break;
        default:
            break;
        }
    }
    return tech;
}
} // namespace raum::scene