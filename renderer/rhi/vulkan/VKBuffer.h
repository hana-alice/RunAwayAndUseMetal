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
    explicit Buffer(const BufferInfo& info, RHIDevice* device);
    explicit Buffer(const BufferSourceInfo& info, RHIDevice* device);
    ~Buffer() override;

    void map(uint32_t offset, uint32_t size) override;
    void unmap() override;
    void* mappedData() const override;

    VkBuffer buffer() const { return _buffer; }
    const VmaAllocationInfo& allocationInfo() { return _allocInfo; }

private:
    Device* _device{nullptr};
    VkBuffer _buffer;
    VmaAllocation _allocation;
    VmaAllocationInfo _allocInfo;
};

struct StagingInfo {
    VkBuffer buffer;
    uint32_t offset;
};

class StagingBuffer {
    struct Chunk {
        uint32_t offset{0};
        uint32_t size{0};
        VkBuffer buffer;
        VmaAllocation allocation;
    };

public:
    StagingBuffer(VmaAllocator alloc);

    StagingInfo alloc(uint32_t size);

    void reset();

    ~StagingBuffer();

private:
    std::vector<Chunk> _chunks;
    VmaAllocator& _allocator;
    static constexpr uint32_t CHUNK_SIZE{1024 * 1024 * 4};
};

} // namespace raum::rhi