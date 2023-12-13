#pragma once
#include "RHIBuffer.h"
#include "vk_mem_alloc.h"
namespace raum::rhi {
class Device;
class Buffer : public RHIBuffer {
public:
    Buffer() = delete;
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    explicit Buffer(const BufferSourceInfo& info, RHIDevice* device);
    explicit Buffer(const BufferInfo& info, RHIDevice* device);
    virtual ~Buffer();

    void update(const void* data, uint32_t size, uint32_t offset = 0);

    VkBuffer buffer() const { return _buffer; }

private:
    Device* _device{nullptr};
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

} // namespace raum::rhi