#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIFrameBuffer {
public:
    explicit RHIFrameBuffer(const FrameBufferInfo&, RHIDevice*){};

protected:
    virtual ~RHIFrameBuffer() = 0;
};

inline RHIFrameBuffer::~RHIFrameBuffer() {}

} // namespace raum::rhi