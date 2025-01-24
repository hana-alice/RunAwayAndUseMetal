#pragma once
#include "RHIComputeEncoder.h"
#include "VKDefine.h"
namespace raum::rhi {
class CommandBuffer;
class ComputePipeline;
class ComputeEncoder : public RHIComputeEncoder {
public:
    explicit ComputeEncoder(CommandBuffer* commandBuffer);
    ComputeEncoder(const ComputeEncoder&) = delete;
    ComputeEncoder& operator=(const ComputeEncoder&) = delete;
    ComputeEncoder(ComputeEncoder&&) = delete;
    ~ComputeEncoder();

    void dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
    void dispatchIndirect(RHIBuffer* indirectBuffer, uint32_t offset) override;
    void bindPipeline(RHIComputePipeline* pipeline) override;
    void bindDescriptorSet(RHIDescriptorSet* descriptorSet, uint32_t index, uint32_t* dynamicOffsets, uint32_t dynOffsetCount) override;
    void pushConstants(ShaderStage stage, uint32_t offset, void* data, uint32_t size) override;

private:
    CommandBuffer* _commandBuffer{nullptr};
    ComputePipeline* _pipeline{nullptr};
};
} // namespace raum::rhi