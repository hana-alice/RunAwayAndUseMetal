#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIFrameBuffer {
public:
    explicit RHIFrameBuffer(const FrameBufferInfo& info, RHIDevice*) : _info(info){};
    FrameBufferInfo info() const { return _info; }

    virtual ~RHIFrameBuffer() = 0;

protected:
    const FrameBufferInfo _info;
};

inline RHIFrameBuffer::~RHIFrameBuffer() {}

} // namespace raum::rhi