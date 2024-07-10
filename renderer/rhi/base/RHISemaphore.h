#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHISemaphore : public RHIResource {
public:
    explicit RHISemaphore(RHIDevice*) {}

    virtual ~RHISemaphore() = 0;
};

inline RHISemaphore::~RHISemaphore() {}

} // namespace raum::rhi