#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIAccelerationStructure: public RHIResource  {
public:
    explicit RHIAccelerationStructure(const AccelerationStructureInfo&, RHIDevice*){};

    virtual ~RHIAccelerationStructure() = 0;

protected:
};

inline RHIAccelerationStructure::~RHIAccelerationStructure() {}

} // namespace raum::rhi