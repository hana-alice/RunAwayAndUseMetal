#pragma once
#include "RHIDefine.h"
namespace raum::render {
class DynamicBuffer {
public:
    explicit DynamicBuffer(rhi::DevicePtr device, uint32_t size, rhi::BufferUsage usage);
    DynamicBuffer(const DynamicBuffer&) = delete;
    DynamicBuffer(DynamicBuffer&&) = default;
    DynamicBuffer& operator=(const DynamicBuffer&) = delete;
    DynamicBuffer& operator=(DynamicBuffer&&) = default;

    virtual ~DynamicBuffer() = default;

    rhi::BufferPtr buffer() const { return _buffer; }
    uint32_t size() const { return _size; }

    void update(const void* data,
                uint32_t size,
                rhi::AccessFlags afterAccess,
                rhi::PipelineStage afterStage,
                rhi::CommandBufferPtr commandBuffer);

private:
    rhi::BufferPtr _buffer{nullptr};
    uint32_t _size{0};
};

class DynamicBufferView {
public:
    explicit DynamicBufferView(rhi::CommandBufferPtr device, const DynamicBuffer& buffer, uint32_t offset, uint32_t size);
    DynamicBufferView(const DynamicBufferView&) = delete;
    DynamicBufferView(DynamicBufferView&&) = default;
    DynamicBufferView& operator=(const DynamicBufferView&) = delete;
    DynamicBufferView& operator=(DynamicBufferView&&) = default;

    virtual ~DynamicBufferView() = default;

    rhi::BufferPtr buffer() const { return _dynamicBuffer.buffer(); }
    uint32_t offset() const { return _offset; }
    uint32_t size() const { return _size; }
    void update(const void* data, uint32_t size, rhi::CommandBufferPtr commandBuffer);
    void update(const void* data, rhi::CommandBufferPtr commandBuffer);

private:
    const DynamicBuffer& _dynamicBuffer;
    uint32_t _offset{0};
    uint32_t _size{0};
};

} // namespace raum::render