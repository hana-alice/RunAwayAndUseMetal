#pragma once
#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHISampler {
public:
    explicit RHISampler(const SamplerInfo& info, RHIDevice*) {}

protected:
    virtual ~RHISampler() = 0;
};

inline RHISampler::~RHISampler() {}

} // namespace raum::rhi