#pragma once
#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHISampler: public RHIResource  {
protected:
    virtual ~RHISampler() = 0;
};

inline RHISampler::~RHISampler() {}

} // namespace raum::rhi