#pragma once
#include "RHIDefine.h"
#include "RHIImage.h"
namespace raum::rhi {

class RHICommandBuffer;

class RHISparseImage : public RHIResource {
public:
    RHISparseImage(const SparseImageInfo& info, RHIDevice* device){};
    virtual void prepare(RHICommandBuffer* cmdBuffer) = 0;
    virtual void update(RHICommandBuffer* cmdBuffer) = 0;
    virtual void setMiptail(uint8_t* data, uint8_t mip) = 0;
    virtual void analyze(RHIBuffer* buffer, RHICommandBuffer* cb) = 0;
    virtual void bind() = 0;

    virtual const Vec3u& granularity() = 0;
};

} // namespace raum::rhi