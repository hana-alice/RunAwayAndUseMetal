#include "DynamicBuffer.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"

namespace raum::render {

DynamicBuffer::DynamicBuffer(rhi::DevicePtr device, uint32_t size, rhi::BufferUsage usage)
: _buffer(nullptr), _size(size) {
    rhi::BufferInfo info{};
    info.size = size;
    info.bufferUsage = usage | rhi::BufferUsage::TRANSFER_DST;

    _buffer = rhi::BufferPtr(device->createBuffer(info));
}



}; // namespace raum::render