#pragma once
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class Queue;
class CommandPool;
class CommandBuffer : public RHICommandBuffer {
public:
    enum class CommandBufferStatus : uint8_t {
        AVAILABLE,
        RENDERING,
        COMPUTING,
        TRANSFERING,
        COMITTED,
    };

    explicit CommandBuffer(const CommandBufferInfo& info, CommandPool*cmdPool, RHIDevice* device);

    RHIRenderEncoder* makeRenderEncoder(RenderEncoderHint hint) override;
    RHIRenderEncoder* makeRenderEncoder() override;
    RHIBlitEncoder* makeBlitEncoder() override;
    RHIComputeEncoder* makeComputeEncoder() override;
    void begin(const CommandBufferBeginInfo& info) override;
    void enqueue(RHIQueue*) override;
    void commit() override;
    void reset() override;
    void appendImageBarrier(const ImageBarrierInfo& info) override;
    void appendBufferBarrier(const BufferBarrierInfo& info) override;
    void appendExecutionBarrier(const ExecutionBarrier& info) override;
    void applyBarrier(DependencyFlags flags) override;
    void onComplete(std::function<void()>&&) override;

    CommandBufferType type() const { return _info.type; }

    VkCommandBuffer commandBuffer() const { return _commandBuffer; }

    ~CommandBuffer();

private:
    bool _enqueued{false};
    CommandBufferStatus _status{CommandBufferStatus::AVAILABLE};
    Device* _device{nullptr};
    Queue* _queue{nullptr};
    CommandPool* _commandPool;
    CommandBufferInfo _info;
    std::vector<ImageBarrierInfo> _imageBarriers;
    std::vector<BufferBarrierInfo> _bufferBarriers;
    std::vector<ExecutionBarrier> _executionBarriers;

    VkCommandBuffer _commandBuffer;
};
} // namespace raum::rhi