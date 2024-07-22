#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIBuffer: public RHIResource {
public:
    explicit RHIBuffer(const BufferInfo& info, RHIDevice*) : _info(info){};
    explicit RHIBuffer(const BufferSourceInfo& info, RHIDevice*) : _info(
                                                                       BufferInfo{
                                                                           MemoryUsage::HOST_VISIBLE,
                                                                           info.sharingMode,
                                                                           info.flag,
                                                                           info.bufferUsage,
                                                                           info.size,
                                                                           info.queueAccess,
                                                                       }){};

    const BufferInfo& info() const { return _info; }

    virtual void map(uint32_t offset, uint32_t size) = 0;
    virtual void unmap() = 0;
    virtual void* mappedData() const = 0;

    virtual ~RHIBuffer() = 0;

protected:
    const BufferInfo _info;
};

inline RHIBuffer::~RHIBuffer() {}

} // namespace raum::rhi