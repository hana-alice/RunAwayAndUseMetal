#pragma once
#include "RHICommandBuffer.h"
#include "VKDefine.h"
#include "RHIDevice.h"
namespace raum::rhi {
class Device;
class Queue;
class CommandBuffer : public RHICommandBuffer {
public:
    enum class CommandBufferStatus : uint8_t {
        AVAILABLE,
        RENDERING,
        COMPUTING,
        TRANSFERING,
        COMITTED,
    };

    explicit CommandBuffer(const CommandBufferInfo& info, RHIQueue* queue, RHIDevice* device);

    RHIRenderEncoder* makeRenderEncoder() override;
    RHIBlitEncoder* makeBlitEncoder() override;
    RHIComputeEncoder* makeComputeEncoder() override;
    void begin(const CommandBufferBeginInfo& info) override;
    void enqueue() override;
    void commit() override;
    void reset() override;
    void appendImageBarrier(const ImageBarrierInfo& info) override;
    void appendBufferBarrier(const BufferBarrierInfo& info) override;
    void applyBarrier(DependencyFlags flags) override;

    CommandBufferType type() const { return _info.type; }

    VkCommandBuffer commandBuffer() const { return _commandBuffer; }
    
    ~CommandBuffer();

private:
    bool _enqueued{false};
    CommandBufferStatus _status{CommandBufferStatus::AVAILABLE};
    Device* _device{nullptr};
    Queue* _queue{nullptr};
    CommandBufferInfo _info;
    std::vector<ImageBarrierInfo> _imageBarriers;
    std::vector<BufferBarrierInfo> _bufferBarriers;

    VkCommandBuffer _commandBuffer;
};
} // namespace raum::rhi