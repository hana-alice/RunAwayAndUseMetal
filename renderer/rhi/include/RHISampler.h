#pragma once
#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHISampler {
public:
    explicit RHISampler(const SamplerInfo& info, RHIDevice*) {}
    virtual ~RHISampler() = 0;
};
} // namespace raum::rhi