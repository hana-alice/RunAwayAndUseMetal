#pragma once
#include "RHIAccelerationStructure.h"

namespace raum::rhi {

class VKAccelerationStructure : public RHIAccelerationStructure {
public:
    VKAccelerationStructure(const AccelerationStructureInfo&, RHIDevice*);
};

}

