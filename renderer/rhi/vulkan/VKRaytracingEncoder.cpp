#include "VKRaytracingEncoder.h"

namespace raum::rhi {
VKRaytracingEncoder::VKRaytracingEncoder(RHICommandBuffer* commandBuffer) : RHIRaytracingEncoder() {
}

void VKRaytracingEncoder::buildAccelerationStructures(
    const std::vector<AccelerationStructureBuildGeometryInfo>& geometries,
    const std::vector<AccelerationStructureBuildRangeInfo>& ranges) {
    // Implementation for building acceleration structures
}

} // namespace raum::rhi