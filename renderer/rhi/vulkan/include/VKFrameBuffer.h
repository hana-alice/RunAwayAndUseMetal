#pragma once
#include "RHIFrameBuffer.h"

namespace raum::rhi {
class Device;
class FrameBuffer : public RHIFrameBuffer {
public:
    explicit FrameBuffer(const FrameBufferInfo& info, RHIDevice* device);
    ~FrameBuffer();

private:
    Device* _device{nullptr};
};
}