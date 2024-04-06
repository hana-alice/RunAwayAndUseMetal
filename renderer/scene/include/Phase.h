#pragma once
#include "RHIDefine.h"
namespace raum::scene {

class Phase;
using PhasePtr = std::shared_ptr<Phase>;

class Phase {
public:
    void setPrimitiveType(rhi::PrimitiveType type);
    void setRasterizationInfo(const rhi::RasterizationInfo& info);
    void setDepthStencilInfo(const rhi::DepthStencilInfo& info);
    void setBlendInfo(const rhi::BlendInfo& info);

    rhi::PrimitiveType primitiveType() const;
    const rhi::RasterizationInfo& rasterizationInfo() const;
    const rhi::DepthStencilInfo& depthStencilInfo() const;
    const rhi::BlendInfo& blendInfo() const;
    const rhi::MultisamplingInfo& multisamplingInfo() const;
    const std::string& phaseName() const;

private:
    std::string _phaseName;
    rhi::PrimitiveType _primitiveType{rhi::PrimitiveType::TRIANGLE_STRIP};
    rhi::RasterizationInfo _rasterizationInfo;
    rhi::DepthStencilInfo _depthStencilInfo;
    rhi::BlendInfo _blendInfo;
    rhi::MultisamplingInfo _multisamplingInfo;

    friend PhasePtr getOrCreatePhase(std::string_view phaseName);
};

PhasePtr getOrCreatePhase(std::string_view phaseName);

} // namespace raum::scene