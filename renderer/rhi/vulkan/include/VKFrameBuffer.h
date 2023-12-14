#pragma once
#include "RHIFrameBuffer.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class FrameBuffer : public RHIFrameBuffer {
public:
    explicit FrameBuffer(const FrameBufferInfo& info, RHIDevice* device);
    ~FrameBuffer();

    VkFramebuffer framebuffer() const { return _framebuffer; }

private:
    Device* _device{nullptr};
    VkFramebuffer _framebuffer;
};
}