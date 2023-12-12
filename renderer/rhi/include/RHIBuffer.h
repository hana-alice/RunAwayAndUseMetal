#pragma once
#include "RHIDefine.h"
namespace raum::rhi {

class RHIBuffer {
public:
    explicit RHIBuffer(const BufferInfo&){};
    explicit RHIBuffer(const BufferSourceInfo&){};

    virtual void update(const void* data, uint32_t dataSize, uint32_t bufferOffset) = 0;
    virtual ~RHIBuffer() = 0;
};

} // namespace raum::rhi