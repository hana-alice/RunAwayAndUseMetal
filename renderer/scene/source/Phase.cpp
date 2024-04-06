#include "Phase.h"

namespace raum::scene {

void Phase::setPrimitiveType(rhi::PrimitiveType type) {
    _primitiveType = type;
}

void Phase::setRasterizationInfo(const rhi::RasterizationInfo &info) {
    _rasterizationInfo = info;
}

void Phase::setDepthStencilInfo(const rhi::DepthStencilInfo &info) {
    _depthStencilInfo = info;
}

void Phase::setBlendInfo(const rhi::BlendInfo &info) {
    _blendInfo = info;
}

rhi::PrimitiveType Phase::primitiveType() const {
    return _primitiveType;
}

const rhi::RasterizationInfo &Phase::rasterizationInfo() const {
    return _rasterizationInfo;
}

const rhi::DepthStencilInfo &Phase::depthStencilInfo() const {
    return _depthStencilInfo;
}

const rhi::BlendInfo &Phase::blendInfo() const {
    return _blendInfo;
}

const rhi::MultisamplingInfo &Phase::multisamplingInfo() const {
    return _multisamplingInfo;
}

const std::string &Phase::phaseName() const {
    return _phaseName;
}

static std::unordered_map<std::string_view, PhasePtr> phases;

PhasePtr getOrCreatePhase(std::string_view phaseName) {
    if(!phases.contains(phaseName)) {
        phases[phaseName.data()] = std::make_shared<Phase>();
        phases[phaseName.data()]->_phaseName = phaseName;
    }
    return phases.at(phaseName.data());
}

} // namespace raum::scene