//
// Created by zeqia on 2024/7/15.
//

#include "VirtualTexture.h"
#include "RHIBlitEncoder.h"
#include "RHIBuffer.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHISparseImage.h"
#include "RHIUtils.h"
namespace raum::render {

constexpr auto MAX_MIP_NUM = 10;

VirtualTexture::VirtualTexture(uint8_t* data, uint32_t width, uint32_t height, rhi::DevicePtr device)
: _device(device) {
    rhi::SparseImageInfo info = {
        .width = width,
        .height = height,
        .maxMip = MAX_MIP_NUM,
        .format = rhi::Format::RGBA8_UNORM,
    };
    _format = info.format;
    _data = data;
    _sparseImage = rhi::SparseImagePtr(_device->createSparseImage(info));
    _width = info.width;
    _height = info.height;

    _granularity = _sparseImage->granularity();
    _pageSize = _granularity.x * _granularity.y * rhi::getFormatSize(info.format);
    _firstMiptail = _sparseImage->firstMipTail();
    _mipProps.resize(_firstMiptail);

    _numCols = (_width + _granularity.x - 1) / _granularity.x;
    _numRows = (_height + _granularity.y - 1) / _granularity.y;

    uint32_t pageCount{0};
    for (uint8_t i = 0; i < _firstMiptail; ++i) {
        uint32_t currMipWidth = width >> i;
        uint32_t currMipHeight = height >> i;
        if (currMipWidth <= 1 || currMipHeight <= 1) break;
        uint32_t w = (currMipWidth + _granularity.x - 1) / _granularity.x;
        uint32_t h = (currMipHeight + _granularity.y - 1) / _granularity.y;
        pageCount += w * h;

        auto& mipProp = _mipProps[i];
        mipProp.width = currMipWidth;
        mipProp.height = currMipHeight;
        mipProp.rowCount = h;
        mipProp.columnCount = w;
        mipProp.pageCount = w * h;
        if (i == 0) {
            mipProp.pageStartIndex = 0;
        } else {
            mipProp.pageStartIndex = _mipProps[i - 1].pageStartIndex + _mipProps[i - 1].pageCount;
        }
    }
    _pageTable.resize(pageCount);

    _sparseImage->initPageInfo(pageCount, _pageSize);

    for (size_t i = 0; i < pageCount; ++i) {
        auto& pt = _pageTable[i];
        pt.mip = getMipLevel(i);

        const auto& mipProp = _mipProps[pt.mip];
        auto& bindInfo = pt.bindInfo;
        bindInfo.x = ((i - mipProp.pageStartIndex) % mipProp.columnCount) * _granularity.x;
        bindInfo.y = ((i - mipProp.pageStartIndex) / mipProp.columnCount) * _granularity.y;
        bindInfo.z = 0;
        bindInfo.w = mipProp.width - bindInfo.x < _granularity.x ? mipProp.width - bindInfo.x : _granularity.x;
        bindInfo.h = mipProp.height - bindInfo.y < _granularity.y ? mipProp.height - bindInfo.y : _granularity.y;
        bindInfo.d = _granularity.z;

        Vec3u offset = {bindInfo.x, bindInfo.y, bindInfo.z};
        Vec3u extent = {bindInfo.w, bindInfo.h, bindInfo.d};
        _sparseImage->setPageMemoryBindInfo(i, offset, extent, pt.mip, 0);
    }

    // shader write - access counter
    rhi::BufferInfo accessBufferInfo{
        .bufferUsage = rhi::BufferUsage::STORAGE | rhi::BufferUsage::TRANSFER_SRC | rhi::BufferUsage::TRANSFER_DST,
        .size = static_cast<uint32_t>(_numRows * _numCols * sizeof(uint32_t)),
    };
    _accessCounter = rhi::BufferPtr(_device->createBuffer(accessBufferInfo));

    std::vector<uint32_t> bufferData(static_cast<uint32_t>(_numRows * _numCols) * rhi::FRAMES_IN_FLIGHT, 255);
    rhi::BufferSourceInfo bsInfo{
        .memUsage = rhi::MemoryUsage::HOST_VISIBLE,
        .bufferUsage = rhi::BufferUsage::TRANSFER_DST,
        .size = static_cast<uint32_t>(bufferData.size() * sizeof(uint32_t)),
        .data = bufferData.data(),
    };
    _feedbackBuffer = rhi::BufferPtr(_device->createBuffer(bsInfo));

    std::array<uint32_t, 4> pageExt = {_numCols, _numRows, _width, _height};
    rhi::BufferSourceInfo pageExtentInfo{
        .size = 4 * sizeof(uint32_t),
        .data = pageExt.data(),
    };
    _metaInfoBuffer = rhi::BufferPtr(_device->createBuffer(pageExtentInfo));

    rhi::ImageViewInfo sparseViewInfo {
        .image = _sparseImage.get(),
        .range = {
            .sliceCount = 1,
            .mipCount = info.maxMip,
        },
        .format = info.format,
    };
    _sparseView = rhi::ImageViewPtr(_device->createImageView(sparseViewInfo));
}

uint8_t VirtualTexture::getMipLevel(uint32_t pageIndex) {
    uint8_t res = 0;
    for (uint8_t i = 0; i < _mipProps.size(); ++i) {
        if (_mipProps[i].pageStartIndex + _mipProps[i].pageCount > pageIndex) {
            res = i;
            break;
        }
    }
    return res;
}

uint32_t VirtualTexture::getPageIndex(uint32_t row, uint32_t col, uint8_t mip) {
    const auto& mipProp = _mipProps[mip];
    return mipProp.pageStartIndex + mipProp.columnCount * col + row;
}

void VirtualTexture::getRelatedBlocks(uint32_t row, uint32_t col, uint8_t mipIn, std::set<size_t>& indices) {
    int8_t mip = mipIn;
    mip -= 1;
    if (mip < 0) return;
    auto minRow = row * 2;
    auto minCol = col * 2;
    auto maxRow = minRow + 1;
    auto maxCol = minCol + 1;
    auto mipProp = _mipProps[mip];
    for (size_t i = minRow; i <= maxRow && i < mipProp.columnCount; ++i) {
        for (size_t j = minCol; j <= maxCol && j < mipProp.rowCount; ++j) {
            auto pageIndex = mipProp.pageStartIndex + j * mipProp.columnCount + i;
            if (!_pageTable[pageIndex].valid) {
                if (mip) {
                    getRelatedBlocks(i, j, mip, indices);
                }
                indices.insert(pageIndex);
            }
        }
    }
}

void VirtualTexture::analyze(rhi::CommandBufferPtr cb) {
    auto frameIndex = _frameCounter.currentValue();
    auto offset = _feedbackBuffer->info().size / rhi::FRAMES_IN_FLIGHT * frameIndex;
    auto* data = static_cast<uint32_t*>(_feedbackBuffer->mappedData()) + offset / sizeof(uint32_t);
    auto width = _numCols;
    auto height = _numRows;
    for (size_t i = 0; i < width; ++i) {
        for (size_t j = 0; j < height; ++j) {
            auto index = j * width + i;
            if (data[index] >= _firstMiptail) {
                continue;
            }
            auto x = i >> data[index];
            auto y = j >> data[index];
            auto pageIndex = getPageIndex(x, y, static_cast<uint8_t>(data[index]));
            if (!_pageTable[pageIndex].valid) {
                _updates.insert(pageIndex);
                getRelatedBlocks(x, y, data[index], _updates);
            } else {
                int a = 0;
            }
            _pageTable[pageIndex].resident = true;
        }
    }

    for (auto pageIndex : _updates) {
        auto& page = _pageTable[pageIndex];
        if (page.valid) {
            continue;
        }
        page.valid = true;

        _sparseImage->allocatePage(pageIndex);

        auto mip = getMipLevel(pageIndex);
        const auto& mipProp = _mipProps[mip];
    }

    ++_frameCounter;
}

void VirtualTexture::invalidateFeedback(rhi::CommandBufferPtr cmd) {
    rhi::ExecutionBarrier executionBarrier;
    executionBarrier.srcStage = rhi::PipelineStage::FRAGMENT_SHADER;
    executionBarrier.dstStage = rhi::PipelineStage::TRANSFER;
    executionBarrier.srcAccessFlag = rhi::AccessFlags::SHADER_WRITE;
    executionBarrier.dstAccessFlag = rhi::AccessFlags::TRANSFER_READ;
    cmd->appendExecutionBarrier(executionBarrier);
    cmd->applyBarrier({});
    auto blit = rhi::BlitEncoderPtr(cmd->makeBlitEncoder());
    rhi::BufferCopyRegion region;
    region.srcOffset = 0;
    region.dstOffset = _frameCounter.currentValue() * _feedbackBuffer->info().size / rhi::FRAMES_IN_FLIGHT;
    region.size = _accessCounter->info().size;
    blit->copyBufferToBuffer(_accessCounter.get(), _feedbackBuffer.get(), &region, 1);
}

void VirtualTexture::resetAccessCounter(rhi::CommandBufferPtr cmd) {
    auto blit = rhi::BlitEncoderPtr(cmd->makeBlitEncoder());
    blit->fillBuffer(_accessCounter.get(), 0, _accessCounter->info().size, 255);
}

void VirtualTexture::prepare(rhi::CommandBufferPtr cb) {
    _sparseImage->prepare(cb.get(), _numCols, _numRows, _pageTable.size(), _pageSize);
}

void VirtualTexture::setMiptail(uint8_t* data, uint8_t mip) {
    _sparseImage->setMiptail(data, mip);
}

void VirtualTexture::update(rhi::CommandBufferPtr cb) {
    analyze(cb);

    if (!_updates.empty() || _remainTask) {
        _remainTask = !_updates.empty();
        auto* queue = _device->getQueue({{rhi::QueueType::GRAPHICS}});
        _sparseImage->bind(rhi::SparseType::OPAQUE | rhi::SparseType::IMAGE);

        std::vector<uint8_t> temp_buffer(_pageSize);

        uint8_t currMip = 0xFF;
        rhi::ImageBarrierInfo barrierInfo{};
        barrierInfo.image = _sparseImage.get();
        rhi::ImageSubresourceRange range{};
        range.aspect = rhi::AspectMask::COLOR;
        range.firstSlice = 0;
        range.sliceCount = 1;
        range.mipCount = 1;
        for (auto pageIndex : _updates) {
            const auto& pt = _pageTable[pageIndex];
            auto mipLevel = pt.mip;

            if (currMip != mipLevel) {
                if (currMip != 0xFF) {
                    range.firstMip = currMip;
                    barrierInfo.range = range;
                    barrierInfo.srcAccessFlag = rhi::AccessFlags::TRANSFER_WRITE;
                    barrierInfo.srcStage = rhi::PipelineStage::TRANSFER;
                    barrierInfo.oldLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL;
                    barrierInfo.dstAccessFlag = rhi::AccessFlags::SHADER_READ;
                    barrierInfo.dstStage = rhi::PipelineStage::FRAGMENT_SHADER;
                    barrierInfo.newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                    cb->appendImageBarrier(barrierInfo);
                    if (currMip != 0) {
                        range.firstMip = currMip - 1;
                        barrierInfo.range = range;
                        barrierInfo.srcAccessFlag = rhi::AccessFlags::TRANSFER_READ;
                        barrierInfo.srcStage = rhi::PipelineStage::TRANSFER;
                        barrierInfo.oldLayout = rhi::ImageLayout::TRANSFER_SRC_OPTIMAL;
                        barrierInfo.dstAccessFlag = rhi::AccessFlags::SHADER_READ;
                        barrierInfo.dstStage = rhi::PipelineStage::FRAGMENT_SHADER;
                        barrierInfo.newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                        cb->appendImageBarrier(barrierInfo);
                    }
                }
                range.firstMip = mipLevel;
                barrierInfo.range = range;
                barrierInfo.srcAccessFlag = rhi::AccessFlags::SHADER_READ;
                barrierInfo.srcStage = rhi::PipelineStage::FRAGMENT_SHADER;
                barrierInfo.oldLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                barrierInfo.dstAccessFlag = rhi::AccessFlags::TRANSFER_WRITE;
                barrierInfo.dstStage = rhi::PipelineStage::TRANSFER;
                barrierInfo.newLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL;
                cb->appendImageBarrier(barrierInfo);
                if (mipLevel != 0) {
                    range.firstMip = mipLevel - 1;
                    barrierInfo.range = range;
                    barrierInfo.srcAccessFlag = rhi::AccessFlags::SHADER_READ;
                    barrierInfo.srcStage = rhi::PipelineStage::FRAGMENT_SHADER;
                    barrierInfo.oldLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                    barrierInfo.dstAccessFlag = rhi::AccessFlags::TRANSFER_READ;
                    barrierInfo.dstStage = rhi::PipelineStage::TRANSFER;
                    barrierInfo.newLayout = rhi::ImageLayout::TRANSFER_SRC_OPTIMAL;
                    cb->appendImageBarrier(barrierInfo);
                }
                currMip = mipLevel;
            }
            cb->applyBarrier({});

            const auto& bindInfo = pt.bindInfo;
            if (mipLevel == 0) {
                for (size_t row = 0; row < bindInfo.h; ++row) {
                    auto formatSize = getFormatSize(_format);
                    auto offset = ((row + bindInfo.y) * _width + bindInfo.x) * formatSize;
                    memcpy(&temp_buffer[row * bindInfo.w * formatSize], _data + offset, bindInfo.w * formatSize);
                }
                rhi::BufferImageCopyRegion region;
                region.bufferOffset = 0;
                region.bufferRowLength = 0;
                region.bufferImageHeight = 0;
                region.imageAspect = rhi::AspectMask::COLOR;
                region.baseMip = 0;
                region.firstSlice = 0;
                region.sliceCount = 1;
                region.imageOffset.x = bindInfo.x;
                region.imageOffset.y = bindInfo.y;
                region.imageOffset.z = bindInfo.z;
                region.imageExtent.x = bindInfo.w;
                region.imageExtent.y = bindInfo.h;
                region.imageExtent.z = bindInfo.d;

                rhi::BufferSourceInfo bufferSourceInfo;
                bufferSourceInfo.data = temp_buffer.data();
                bufferSourceInfo.size = temp_buffer.size();
                bufferSourceInfo.bufferUsage = rhi::BufferUsage::TRANSFER_SRC;
                bufferSourceInfo.queueAccess = {queue->index()};

                rhi::BufferPtr bufferPtr = rhi::BufferPtr(_device->createBuffer(bufferSourceInfo));
                auto blit = rhi::BlitEncoderPtr(cb->makeBlitEncoder());
                rhi::BufferImageCopyRegion copy{
                    .bufferOffset = 0,
                    .bufferRowLength = 0,
                    .bufferImageHeight = 0,
                    .imageAspect = rhi::AspectMask::COLOR,
                    .baseMip = 0,
                    .firstSlice = 0,
                    .sliceCount = 1,
                    .imageOffset = {bindInfo.x, bindInfo.y, bindInfo.z},
                    .imageExtent = {bindInfo.w, bindInfo.h, bindInfo.d},
                };
                blit->copyBufferToImage(bufferPtr.get(), _sparseImage.get(), rhi::ImageLayout::TRANSFER_DST_OPTIMAL, &copy, 1);

                cb->onComplete([bufferPtr]() mutable {
                    bufferPtr.reset();
                });
            } else {
                auto blit = rhi::BlitEncoderPtr(cb->makeBlitEncoder());

                rhi::ImageBlit region{
                    .srcImageAspect = rhi::AspectMask::COLOR,
                    .dstImageAspect = rhi::AspectMask::COLOR,
                    .srcOffset = {bindInfo.x * 2, bindInfo.y * 2, bindInfo.z},
                    .dstOffset = {bindInfo.x, bindInfo.y, bindInfo.z},
                    .srcBaseMip = static_cast<uint32_t>(mipLevel - 1),
                    .srcFirstSlice = 0,
                    .dstBaseMip = mipLevel,
                    .dstFirstSlice = 0,
                    .sliceCount = 1,
                    .srcExtent = {bindInfo.w * 2, bindInfo.h * 2, bindInfo.d},
                    .dstExtent = {bindInfo.w, bindInfo.h, bindInfo.d},
                };
                blit->blitImage(_sparseImage.get(), rhi::ImageLayout::TRANSFER_SRC_OPTIMAL, _sparseImage.get(), rhi::ImageLayout::TRANSFER_DST_OPTIMAL, &region, 1, rhi::Filter::LINEAR);
            }
        }

        for (size_t i = 0; i < _pageTable.size(); ++i) {
            auto& pt = _pageTable[i];
            if (!pt.resident && !pt.valid) continue;
            pt.valid = pt.resident;
            if (!pt.valid) {
                _sparseImage->reset(i);
            }
            pt.resident = false;
        }

        _updates.clear();

        range.aspect = rhi::AspectMask::COLOR;
        range.firstSlice = 0;
        range.sliceCount = 1;
        range.mipCount = 1;
        if (currMip != 0xFF) {
            range.firstMip = currMip;
            barrierInfo.range = range;
            barrierInfo.srcAccessFlag = rhi::AccessFlags::TRANSFER_WRITE;
            barrierInfo.srcStage = rhi::PipelineStage::TRANSFER;
            barrierInfo.oldLayout = rhi::ImageLayout::TRANSFER_DST_OPTIMAL;
            barrierInfo.dstAccessFlag = rhi::AccessFlags::SHADER_READ;
            barrierInfo.dstStage = rhi::PipelineStage::FRAGMENT_SHADER;
            barrierInfo.newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
            cb->appendImageBarrier(barrierInfo);
            if (currMip != 0) {
                range.firstMip = currMip - 1;
                barrierInfo.range = range;
                barrierInfo.srcAccessFlag = rhi::AccessFlags::TRANSFER_READ;
                barrierInfo.srcStage = rhi::PipelineStage::TRANSFER;
                barrierInfo.oldLayout = rhi::ImageLayout::TRANSFER_SRC_OPTIMAL;
                barrierInfo.dstAccessFlag = rhi::AccessFlags::SHADER_READ;
                barrierInfo.dstStage = rhi::PipelineStage::FRAGMENT_SHADER;
                barrierInfo.newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                cb->appendImageBarrier(barrierInfo);
            }
            cb->applyBarrier({});
        }
    }
}

VirtualTexture::~VirtualTexture() {

}

} // namespace raum::render