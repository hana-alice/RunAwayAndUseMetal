#pragma once
#include <vector>
#include "RHIDefine.h"

namespace raum::rhi {

class RHIAccelerationStructure;

class RHIRaytracingEncoder {
    public:
    virtual ~RHIRaytracingEncoder() = 0;

    virtual void buildAccelerationStructures(
        const std::vector<AccelerationStructureBuildGeometryInfo>&,
        const std::vector<AccelerationStructureBuildRangeInfo>&) = 0;

};

inline RHIRaytracingEncoder::~RHIRaytracingEncoder() {

}


}