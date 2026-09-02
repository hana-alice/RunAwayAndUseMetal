#include "RHIStagingBuffer.h"
#include <algorithm>
#include "RHIDevice.h"

namespace raum::rhi {
RHIStagingBuffer::RHIStagingBuffer(uint32_t chunkSize, RHIDevice* device)
    : _chunkSize(chunkSize), _device(device) {
}

StagingBufferInfo RHIStagingBuffer::allocate(uint32_t size) {
    StagingBufferInfo* target{nullptr};
    for (auto& buffer : _buffers) {
        if (buffer.offset + size <= buffer.size) {
            target = &buffer;
            break;
        }
    }
    if (!target) {
        auto& buffer = _buffers.emplace_back(StagingBufferInfo{});
        BufferInfo bufferInfo{
            .memUsage = MemoryUsage::STAGING,
            .bufferUsage = BufferUsage::TRANSFER_SRC,
            .size = std::max(_chunkSize, size),
        };
        buffer.buffer = BufferPtr(_device->createBuffer(bufferInfo));
        buffer.size = bufferInfo.size;
        buffer.offset = 0;
        target = &buffer;
    }
    auto offset = target->offset;
    target->offset = offset + size;
    return {target->buffer, offset, size};
}

void RHIStagingBuffer::reset() {
    for (auto& buffer : _buffers) {
        buffer.offset = 0;
    }
}
} // namespace raum::rhi
