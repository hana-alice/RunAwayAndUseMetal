#include "Technique.h"

namespace raum::scene {

Technique::Technique(MaterialPtr material, std::string_view phaseName)
:_material(material), _phaseName(phaseName){
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

void Technique::bakePipelineState() {

}

rhi::GraphicsPipelinePtr Technique::pipelineState() {
    return _pso;
}

}
