#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIBuffer {
public:
    explicit RHIBuffer(const BufferInfo& info, RHIDevice*) : _info(info){};

    const BufferInfo& info() const { return _info; }

    virtual ~RHIBuffer() = 0;

protected:
    const BufferInfo _info;
};

inline RHIBuffer::~RHIBuffer() {}

} // namespace raum::rhi