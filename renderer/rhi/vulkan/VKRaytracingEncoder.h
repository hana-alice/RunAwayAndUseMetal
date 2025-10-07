#pragma once
#include "RHIRaytracingEncoder.h"
namespace raum::rhi {
class VKRaytracingEncoder : public RHIRaytracingEncoder {
public:
    VKRaytracingEncoder(RHICommandBuffer*);

    void buildAccelerationStructures(
        const std::vector<AccelerationStructureBuildGeometryInfo>&,
        const std::vector<AccelerationStructureBuildRangeInfo>&) override;
};
} // namespace raum::rhi