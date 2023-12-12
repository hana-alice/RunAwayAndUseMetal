#pragma once
#include "VKDefine.h"
#include "vk_mem_alloc.h"
namespace raum::rhi {
class Buffer {
public:
    Buffer() = delete;
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    explicit Buffer(const BufferSourceInfo& info);
    explicit Buffer(const BufferInfo& info);
    virtual ~Buffer();

    void update(const void* data, uint32_t size, uint32_t offset = 0);

    VkBuffer buffer() const { return _buffer; }

private:
    VkBuffer _buffer;
    VmaAllocation _allocation;
};

class VertexBuffer : public Buffer {
public:
    explicit VertexBuffer(const BufferSourceInfo& info, uint32_t stride);
    explicit VertexBuffer(const BufferInfo& info, uint32_t stride);

    ~VertexBuffer();

private:
    uint32_t _stride{0};
};

class IndexBuffer : public Buffer {
public:
    explicit IndexBuffer(const BufferSourceInfo& info);
    explicit IndexBuffer(const BufferInfo& info);

private:
    IndexType _indexType{IndexType::HALF};
};

class UniformBuffer : public Buffer {
public:
};

class IndirectBuffer : public Buffer {
};
} // namespace raum::rhi