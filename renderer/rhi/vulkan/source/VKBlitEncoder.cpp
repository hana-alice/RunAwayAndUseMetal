#include "VKBlitEncoder.h"
#include "VKBuffer.h"
#include "VKCommandBuffer.h"
#include "VKImage.h"
#include "VKUtils.h"
namespace raum::rhi {

BlitEncoder::BlitEncoder(CommandBuffer* commandBuffer) : _commandBuffer(commandBuffer) {
}

BlitEncoder::~BlitEncoder() {
}

void BlitEncoder::copyBufferToBuffer(RHIBuffer* srcBuffer, RHIBuffer* dstBuffer, BufferCopyRegion* regions, uint32_t regionCount) {
    auto src = static_cast<Buffer*>(srcBuffer)->buffer();
    auto dst = static_cast<Buffer*>(dstBuffer)->buffer();
    std::vector<VkBufferCopy> regionCopies(regionCount);
    for (size_t i = 0; i < regionCount; ++i) {
        VkBufferCopy regionCopy{};
        regionCopy.dstOffset = regions[i].dstOffset;
        regionCopy.srcOffset = regions[i].srcOffset;
        regionCopy.size = regions[i].size;
        regionCopies[i] = regionCopy;
    }
    vkCmdCopyBuffer(_commandBuffer->commandBuffer(), src, dst, regionCount, regionCopies.data());
}

void BlitEncoder::copyImageToImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageCopyRegion* regions, uint32_t regionCount) {
    auto src = static_cast<Image*>(srcImage)->image();
    auto dst = static_cast<Image*>(dstImage)->image();
    VkImageLayout srcL = imageLayout(srcLayout);
    VkImageLayout dstL = imageLayout(dstLayout);

    std::vector<VkImageCopy> regionsCopies(regionCount);
    for (size_t i = 0; i < regionCount; ++i) {
        VkImageCopy copy{};
        const auto& dstOffset = regions[i].dstOffset;
        copy.dstOffset = {
            static_cast<int32_t>(dstOffset.x),
            static_cast<int32_t>(dstOffset.y),
            static_cast<int32_t>(dstOffset.z)};
        const auto& srcOffset = regions[i].srcOffset;
        copy.srcOffset = {
            static_cast<int32_t>(srcOffset.x),
            static_cast<int32_t>(srcOffset.y),
            static_cast<int32_t>(srcOffset.z),
        };

        copy.dstSubresource.aspectMask = aspectMask(regions[i].dstImageAspect);
        copy.dstSubresource.baseArrayLayer = regions[i].dstFirstSlice;
        copy.dstSubresource.layerCount = regions[i].sliceCount;
        copy.dstSubresource.mipLevel = regions[i].dstBaseMip;

        copy.srcSubresource.aspectMask = aspectMask(regions[i].srcImageAspect);
        copy.srcSubresource.baseArrayLayer = regions[i].srcFirstSlice;
        copy.srcSubresource.layerCount = regions[i].sliceCount;
        copy.srcSubresource.mipLevel = regions[i].srcBaseMip;

        copy.extent.width = regions[i].extent.x;
        copy.extent.height = regions[i].extent.y;
        copy.extent.depth = regions[i].extent.z;
        regionsCopies[i] = copy;
    }

    vkCmdCopyImage(_commandBuffer->commandBuffer(), src, srcL, dst, dstL, regionCount, regionsCopies.data());
}

void BlitEncoder::blitImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageBlit* regions, uint32_t regionCount, Filter filter) {
    auto src = static_cast<Image*>(srcImage)->image();
    auto dst = static_cast<Image*>(dstImage)->image();

    VkImageLayout srcL = imageLayout(srcLayout);
    VkImageLayout dstL = imageLayout(dstLayout);

    std::vector<VkImageBlit> blits(regionCount);
    for (size_t i = 0; i < regionCount; ++i) {
        VkImageBlit blit{};
        const auto& srcOffset = regions[i].srcOffset;
        blit.srcOffsets[0] = {
            static_cast<int32_t>(srcOffset.x),
            static_cast<int32_t>(srcOffset.y),
            static_cast<int32_t>(srcOffset.z),
        };
        const auto& srcExtent = regions[i].srcExtent;
        blit.srcOffsets[1] = {
            static_cast<int32_t>(srcExtent.x),
            static_cast<int32_t>(srcExtent.y),
            static_cast<int32_t>(srcExtent.z),
        };
        const auto& dstOffset = regions[i].dstOffset;
        blit.dstOffsets[0] = {
            static_cast<int32_t>(dstOffset.x),
            static_cast<int32_t>(dstOffset.y),
            static_cast<int32_t>(dstOffset.z),
        };
        const auto& dstExtent = regions[i].dstExtent;
        blit.dstOffsets[1] = {
            static_cast<int32_t>(dstExtent.x),
            static_cast<int32_t>(dstExtent.y),
            static_cast<int32_t>(dstExtent.z),
        };
        blit.srcSubresource.aspectMask = aspectMask(regions[i].srcImageAspect);
        blit.srcSubresource.baseArrayLayer = regions[i].srcFirstSlice;
        blit.srcSubresource.layerCount = regions[i].sliceCount;
        blit.srcSubresource.mipLevel = regions[i].srcBaseMip;
        blit.dstSubresource.aspectMask = aspectMask(regions[i].dstImageAspect);
        blit.dstSubresource.baseArrayLayer = regions[i].dstFirstSlice;
        blit.dstSubresource.layerCount = regions[i].sliceCount;
        blit.dstSubresource.mipLevel = regions[i].dstBaseMip;
        blits[i] = blit;
    }

    VkFilter flt = mapFilter(filter);

    vkCmdBlitImage(_commandBuffer->commandBuffer(), src, srcL, dst, dstL, regionCount, blits.data(), flt);
}

void BlitEncoder::copyBufferToImage(RHIBuffer* buffer, RHIImage* image, ImageLayout layout, BufferImageCopyRegion* regions, uint32_t regionCount) {
    auto src = static_cast<Buffer*>(buffer)->buffer();
    auto dst = static_cast<Image*>(image)->image();
    VkImageLayout imgLayout = imageLayout(layout);

    std::vector<VkBufferImageCopy> copies(regionCount);
    for (size_t i = 0; i < regionCount; ++i) {
        VkBufferImageCopy copy{};
        copy.bufferImageHeight = regions[i].bufferImageHeight;
        copy.bufferRowLength = regions[i].bufferRowLength;
        copy.bufferOffset = regions[i].bufferOffset;
        const auto& imageOffset = copy.imageOffset;
        copy.imageOffset = {
            static_cast<int32_t>(imageOffset.x),
            static_cast<int32_t>(imageOffset.y),
            static_cast<int32_t>(imageOffset.z),
        };
        const auto& imageExtent = regions[i].imageExtent;
        copy.imageExtent = {
            static_cast<uint32_t>(imageExtent.x),
            static_cast<uint32_t>(imageExtent.y),
            static_cast<uint32_t>(imageExtent.z),
        };
        copy.imageSubresource = {
            aspectMask(regions[i].imageAspect),
            regions[i].baseMip,
            regions[i].firstSlice,
            regions[i].sliceCount,
        };
        copies[i] = copy;
    }

    vkCmdCopyBufferToImage(_commandBuffer->commandBuffer(), src, dst, imgLayout, regionCount, copies.data());
}

void BlitEncoder::copyImageToBuffer(RHIImage* image, ImageLayout layout, RHIBuffer* dstBuffer, BufferImageCopyRegion* regions, uint32_t regionCount) {
    auto src = static_cast<Image*>(image)->image();
    auto dst = static_cast<Buffer*>(dstBuffer)->buffer();
    VkImageLayout imgLayout = imageLayout(layout);

    std::vector<VkBufferImageCopy> copies(regionCount);
    for (size_t i = 0; i < regionCount; ++i) {
        VkBufferImageCopy copy{};
        copy.bufferImageHeight = regions[i].bufferImageHeight;
        copy.bufferRowLength = regions[i].bufferRowLength;
        copy.bufferOffset = regions[i].bufferOffset;
        const auto& imageOffset = copy.imageOffset;
        copy.imageOffset = {
            static_cast<int32_t>(imageOffset.x),
            static_cast<int32_t>(imageOffset.y),
            static_cast<int32_t>(imageOffset.z),
        };
        const auto& imageExtent = regions[i].imageExtent;
        copy.imageExtent = {
            static_cast<uint32_t>(imageExtent.x),
            static_cast<uint32_t>(imageExtent.y),
            static_cast<uint32_t>(imageExtent.z),
        };
        copy.imageSubresource = {
            aspectMask(regions[i].imageAspect),
            regions[i].baseMip,
            regions[i].firstSlice,
            regions[i].sliceCount,
        };
        copies[i] = copy;
    }

    vkCmdCopyImageToBuffer(_commandBuffer->commandBuffer(), src, imgLayout, dst, regionCount, copies.data());
}

void BlitEncoder::updateBuffer(RHIBuffer* buffer, uint32_t dstOffset, void* data, uint32_t dataSize) {
    RAUM_WARN_IF(buffer->info().size > 65536, "Use copy staging buffer instead!");

    auto buf = static_cast<Buffer*>(buffer)->buffer();
    vkCmdUpdateBuffer(_commandBuffer->commandBuffer(), buf, dstOffset, dataSize, data);
}

void BlitEncoder::fillBuffer(RHIBuffer* buffer, uint32_t dstOffset, uint32_t size, uint32_t value) {
    auto buf = static_cast<Buffer*>(buffer)->buffer();
    vkCmdFillBuffer(_commandBuffer->commandBuffer(), buf, dstOffset, size, value);
}

void BlitEncoder::clearColorImage(RHIImage* image, ImageLayout layout, ClearColor* data, ImageSubresourceRange* ranges, uint32_t rangeCount) {
    auto img = static_cast<Image*>(image)->image();
    VkImageLayout imgLayout = imageLayout(layout);

    std::vector<VkClearColorValue> clearValues(rangeCount);
    for (size_t i = 0; i < rangeCount; ++i) {
        fillClearColors(clearValues, data, image->info().format);
    }

    std::vector<VkImageSubresourceRange> subranges(rangeCount);
    for (size_t i = 0; i < rangeCount; ++i) {
        subranges[i].aspectMask = aspectMask(ranges[i].aspect);
        subranges[i].baseArrayLayer = ranges[i].firstSlice;
        subranges[i].layerCount = ranges[i].sliceCount;
        subranges[i].baseMipLevel = ranges[i].firstMip;
        subranges[i].levelCount = ranges[i].mipCount;
    }

    vkCmdClearColorImage(_commandBuffer->commandBuffer(), img, imgLayout, clearValues.data(), rangeCount, subranges.data());
}

void BlitEncoder::clearDepthStencilImage(RHIImage* image, ImageLayout layout, float depth, uint32_t stencil, ImageSubresourceRange* ranges, uint32_t rangeCount) {
    auto img = static_cast<Image*>(image)->image();
    VkImageLayout imgLayout = imageLayout(layout);

    VkClearDepthStencilValue clearValue{depth, stencil};

    std::vector<VkImageSubresourceRange> subranges(rangeCount);
    for (size_t i = 0; i < rangeCount; ++i) {
        subranges[i].aspectMask = aspectMask(ranges[i].aspect);
        subranges[i].baseArrayLayer = ranges[i].firstSlice;
        subranges[i].layerCount = ranges[i].sliceCount;
        subranges[i].baseMipLevel = ranges[i].firstMip;
        subranges[i].levelCount = ranges[i].mipCount;
    }
    vkCmdClearDepthStencilImage(_commandBuffer->commandBuffer(), img, imgLayout, &clearValue, rangeCount, subranges.data());
}

void BlitEncoder::resolveImage(RHIImage* srcImage, ImageLayout srcLayout, RHIImage* dstImage, ImageLayout dstLayout, ImageResolve* regions, uint32_t regionCount) {
    auto src = static_cast<Image*>(srcImage)->image();
    auto dst = static_cast<Image*>(dstImage)->image();
    VkImageLayout srcL = imageLayout(srcLayout);
    VkImageLayout dstL = imageLayout(dstLayout);
    std::vector<VkImageResolve> resolves(regionCount);
    for (size_t i = 0; i < regionCount; ++i) {
        auto& res = resolves[i];
        res.srcOffset = {
            static_cast<int32_t>(regions[i].srcOffset.x),
            static_cast<int32_t>(regions[i].srcOffset.y),
            static_cast<int32_t>(regions[i].srcOffset.z),
        };
        res.dstOffset = {
            static_cast<int32_t>(regions[i].dstOffset.x),
            static_cast<int32_t>(regions[i].dstOffset.y),
            static_cast<int32_t>(regions[i].dstOffset.z),
        };
        res.srcSubresource = {
            aspectMask(regions[i].srcImageAspect),
            regions[i].srcBaseMip,
            regions[i].srcFirstSlice,
            regions[i].sliceCount,
        };
        res.dstSubresource = {
            aspectMask(regions[i].dstImageAspect),
            regions[i].dstBaseMip,
            regions[i].dstFirstSlice,
            regions[i].sliceCount,
        };
        res.extent = {
            regions[i].extent.x,
            regions[i].extent.y,
            regions[i].extent.z,
        };
    }

    vkCmdResolveImage(_commandBuffer->commandBuffer(), src, srcL, dst, dstL, regionCount, resolves.data());
}

} // namespace raum::rhi