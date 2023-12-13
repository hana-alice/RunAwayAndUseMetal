#include "VKFrameBuffer.h"
#include "VKDevice.h"
namespace raum::rhi {
FrameBuffer::FrameBuffer(const FrameBufferInfo& info, RHIDevice* device)
: RHIFrameBuffer(info, device), _device(static_cast<Device*>(device)) {
    VkFramebufferCreateInfo createInfo{};
    createInfo.width = info.width;
    createInfo.height = info.height;
    createInfo.layers = info.layers;



}

FrameBuffer::~FrameBuffer() {
}
}