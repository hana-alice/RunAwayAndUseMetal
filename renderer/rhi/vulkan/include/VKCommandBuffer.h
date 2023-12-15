#pragma once
#include "RHICommandBuffer.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;

class CommandBuffer : public RHICommandBuffer {
public:
    enum class CommandBufferStatus : uint8_t {
        AVAILABLE,
        RENDERING,
        COMPUTING,
        TRANSFERING,
        COMITTED,
    };

    explicit CommandBuffer(const CommandBufferInfo& info, RHIDevice* device);

    RHIRenderEncoder* makeRenderEncoder() override;
    RHIBlitEncoder* makeBlitEncoder() override;
    RHIComputeEncoder* makeComputeEncoder() override;
    void enqueue() override;
    void commit() override;
    void reset() override;

    CommandBufferType type() const { return _info.type; }

    VkCommandBuffer commandBuffer() const { return _commandBuffer; }
    
    ~CommandBuffer();

private:
    bool _enqueued{false};
    CommandBufferStatus _status{CommandBufferStatus::AVAILABLE};
    Device* _device{nullptr};
    CommandBufferInfo _info;

    VkCommandBuffer _commandBuffer;
};
} // namespace raum::rhi