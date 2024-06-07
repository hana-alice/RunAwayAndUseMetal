#include "RHIBlitEncoder.h"
#include "VKDefine.h"
namespace raum::rhi {

class CommandBuffer;
class BlitEncoder : public RHIBlitEncoder {
public:
    BlitEncoder(CommandBuffer* commandBuffer);
    ~BlitEncoder() override;

    void copyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, BufferCopyRegion* regions, uint32_t regionCount) override;
    // bit-wise
    void copyImageToImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageCopyRegion* regions, uint32_t regionCount) override;
    // scalable
    void blitImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageBlit* regions, uint32_t regionCount, Filter filter) override;
    void copyBufferToImage(RHIBuffer* buffer, RHIImage* image, ImageLayout layout, BufferImageCopyRegion* regions, uint32_t regionCount) override;
    void copyImageToBuffer(RHIImage* image, ImageLayout layout, RHIBuffer* dstBuffer, BufferImageCopyRegion* regions, uint32_t regionCount) override;
    void updateBuffer(RHIBuffer* buffer, uint32_t dstOffset, const void* const data, uint32_t dataSize) override;
    void fillBuffer(RHIBuffer* buffer, uint32_t dstOffset, uint32_t size, uint32_t value) override;

    void clearColorImage(RHIImage* image, ImageLayout layout, ClearValue* data, ImageSubresourceRange* ranges, uint32_t rangeCount) override;
    void clearDepthStencilImage(RHIImage* image, ImageLayout layout, float depth, uint32_t stencil, ImageSubresourceRange* ranges, uint32_t rangeCount) override;

    void resolveImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageResolve* regions, uint32_t regionCount) override;

private:
    CommandBuffer* _commandBuffer{nullptr};
};

}