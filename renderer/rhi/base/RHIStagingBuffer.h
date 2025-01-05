#pragma once
#include "RHIBuffer.h"
namespace raum::rhi {
class RHIDevice;

class RHIStagingBuffer final : public RHIResource {
public:
    explicit RHIStagingBuffer(uint32_t chunkSize, RHIDevice* device);
    ~RHIStagingBuffer() override {};

    StagingBufferInfo allocate(uint32_t size);
    void reset();

private:
    uint32_t _chunkSize{0};
    RHIDevice* _device{nullptr};
    uint32_t _currentIndex{0};
    std::vector<StagingBufferInfo> _buffers;
};


} // namespace raum::rhi