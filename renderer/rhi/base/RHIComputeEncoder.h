#pragma once
#include "RHIDefine.h"

namespace raum::rhi {

class RHIComputePipeline;
class RHIDescriptorSet;
class RHIBuffer;
class RHIComputeEncoder {
public:
    virtual ~RHIComputeEncoder() = 0;

    virtual void bindPipeline(RHIComputePipeline* pipeline) = 0;
    virtual void bindDescriptorSet(RHIDescriptorSet* descriptorSet, uint32_t index, uint32_t* dynamicOffsets, uint32_t dynOffsetCount) = 0;
    virtual void dispatch(uint32_t groupX, uint32_t groupY, uint32_t groupZ) = 0;
    virtual void dispatchIndirect(RHIBuffer* indirectBuffer, uint32_t offset) = 0;
    virtual void pushConstants(ShaderStage stage, uint32_t offset, void* data, uint32_t size) = 0;
};

inline RHIComputeEncoder::~RHIComputeEncoder() {}

} // namespace raum::rhi