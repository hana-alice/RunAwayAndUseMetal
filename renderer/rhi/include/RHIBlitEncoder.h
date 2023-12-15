#pragma once
#include "RHIDefine.h"

namespace raum::rhi {

class RHIBuffer;
class RHIImage;
class RHIBlitEncoder {
public:
    virtual ~RHIBlitEncoder() = 0;

    virtual void copyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, BufferCopyRegion* regions, uint32_t regionCount) = 0;
    // bit-wise
    virtual void copyImageToImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageCopyRegion* regions, uint32_t regionCount) = 0;
    // scalable
    virtual void blitImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageCopyRegion* regions, uint32_t regionCount, Filter filter) = 0;
    virtual void copyBufferToImage(RHIBuffer* buffer, RHIImage* image, ImageLayout layout, BufferImageCopyRegion* regions, uint32_t regionCount) = 0;
    virtual void copyImageToBuffer(RHIImage* image, ImageLayout layout, RHIBuffer* dstBuffer, BufferImageCopyRegion* regions, uint32_t regionCount) = 0;
    virtual void updateBuffer(RHIBuffer* buffer, uint32_t dstOffset, void* data, uint32_t dataSize) = 0;
    virtual void fillBuffer(RHIBuffer* buffer, uint32_t dstOffset, uint32_t size, uint32_t value) = 0;

    virtual void clearColorImage(RHIImage* image, ImageLayout layout, ClearColor* data, Range* ranges, uint32_t rangeCount) = 0;

    virtual void resolveImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageCopyRegion* regions, uint32_t regionCount) = 0;
};

inline RHIBlitEncoder::~RHIBlitEncoder() {}

} // namespace raum::rhi