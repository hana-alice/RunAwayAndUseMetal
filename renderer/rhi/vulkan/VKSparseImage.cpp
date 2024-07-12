#include "VKSparseImage.h"
#include "RHIUtils.h"
#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKQueue.h"
#include "VKUtils.h"

namespace raum::rhi {

constexpr auto MAX_MIP_NUM = 10;
uint32_t SparseImage::MemAllocator::MemoryTypeIndex = 0xFFFFFFFF;
uint32_t SparseImage::MemAllocator::PageSize = 0xFFFFFFFF;
uint32_t SparseImage::MemAllocator::PagesPerAlloc = 0xFFFFFFFF;

SparseImage::SparseImage(const SparseImageInfo& info, Device* dev)
: RHISparseImage(info, dev), _device(static_cast<Device*>(dev)), _data(info.data) {
    uint32_t width = info.width;
    uint32_t height = info.height;

    _format = info.format;
    _width = info.width;
    _height = info.height;

    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.format = formatInfo(info.format).format;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.flags = VK_IMAGE_CREATE_SPARSE_BINDING_BIT | VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
    createInfo.extent.width = width;
    createInfo.extent.height = height;
    createInfo.extent.depth = 1;
    createInfo.mipLevels = MAX_MIP_NUM;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocInfo.priority = 1.0f;

    VkResult res = vkCreateImage(_device->device(), &createInfo, nullptr, &_sparseImage);
    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create image.");

    std::vector<VkSparseImageMemoryRequirements> reqs;
    uint32_t count;
    vkGetImageSparseMemoryRequirements(_device->device(), _sparseImage, &count, nullptr);
    reqs.resize(count);
    vkGetImageSparseMemoryRequirements(_device->device(), _sparseImage, &count, reqs.data());
    vkGetImageMemoryRequirements(_device->device(), _sparseImage, &_memReq);
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(_device->physicalDevice(), &memProps);

    _req = reqs.front();

    auto bits = _memReq.memoryTypeBits;
    uint32_t typeIndex{0xFFFFFFFF};
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((bits & 1) == 1) {
            if ((memProps.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                typeIndex = i;
            }
        }
        bits >>= 1;
    }
    raum_check(typeIndex != 0xFFFFFFFF, "can't find request memory type");
    if (SparseImage::MemAllocator::MemoryTypeIndex == 0xFFFFFFFF) {
        SparseImage::MemAllocator::MemoryTypeIndex = typeIndex;
    }

    _granularity = {
        reqs[0].formatProperties.imageGranularity.width,
        reqs[0].formatProperties.imageGranularity.height,
        reqs[0].formatProperties.imageGranularity.depth,
    };

    _vt.pageSize = _granularity.x * _granularity.y * getFormatSize(info.format);
    _vt.mips.resize(reqs[0].imageMipTailFirstLod);

    if (SparseImage::MemAllocator::PageSize == 0xFFFFFFFF) {
        SparseImage::MemAllocator::PageSize = _vt.pageSize;
    }

    uint32_t pageCount{0};
    for (size_t i = 0; i < reqs[0].imageMipTailFirstLod; ++i) {
        uint32_t currMipWidth = width >> i;
        uint32_t currMipHeight = height >> i;
        if (currMipWidth <= 1 || currMipHeight <= 1) break;
        uint32_t w = (currMipWidth + _granularity.x - 1) / _granularity.x;
        uint32_t h = (currMipHeight + _granularity.y - 1) / _granularity.y;
        pageCount += w * h;

        auto& mipProp = _vt.mips[i];
        mipProp.width = currMipWidth;
        mipProp.height = currMipHeight;
        mipProp.rowCount = h;
        mipProp.columnCount = w;
        mipProp.pageCount = w * h;
        if (i == 0) {
            mipProp.pageStartIndex = 0;
        } else {
            mipProp.pageStartIndex = _vt.mips[i - 1].pageStartIndex + _vt.mips[i - 1].pageCount;
        }
    }
    _vt.pageTable.resize(pageCount);
    _vt.sparseImageMemoryBinds.resize(pageCount, {});

    // bind tail
    auto& req = reqs.front();
    auto size = req.imageMipTailSize;

    VkMemoryRequirements memrequires{
        .size = size,
        .alignment = _memReq.alignment,
        .memoryTypeBits = _memReq.memoryTypeBits,
    };
    VmaAllocationInfo ai;
    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    aci.memoryTypeBits = memrequires.memoryTypeBits;
    res = vmaAllocateMemoryPages(_device->allocator(), &memrequires, &aci, 1, &_miptailAlloc, &ai);
    assert(res == VK_SUCCESS);

    _vt.miptailBind = {};
    _vt.miptailBind.size = size;
    _vt.miptailBind.resourceOffset = req.imageMipTailOffset;
    _vt.miptailBind.memory = ai.deviceMemory;
    _vt.miptailBind.memoryOffset = ai.offset;

    constexpr uint32_t pagesPerAlloc = 50;
    if (SparseImage::MemAllocator::PagesPerAlloc == 0xFFFFFFFF) {
        SparseImage::MemAllocator::PagesPerAlloc = pagesPerAlloc;
    }

    memAllocInfo.pageSize = _vt.pageSize;
    memAllocInfo.pagesPerAlloc = pagesPerAlloc;

    for (size_t i = 0; i < _vt.pageTable.size(); ++i) {
        uint8_t mipLevel = getMipLevel(i);
        auto& memBindInfo = _vt.sparseImageMemoryBinds[i];
        auto& mipProp = _vt.mips[mipLevel];

        memBindInfo.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        memBindInfo.subresource.mipLevel = mipLevel;
        memBindInfo.subresource.arrayLayer = 0;
        memBindInfo.offset.x = ((i - mipProp.pageStartIndex) % mipProp.columnCount) * _granularity.x;
        memBindInfo.offset.y = ((i - mipProp.pageStartIndex) / mipProp.columnCount) * _granularity.y;
        memBindInfo.offset.z = 0;

        memBindInfo.extent.width = mipProp.width - memBindInfo.offset.x < _granularity.x ? mipProp.width - memBindInfo.offset.x : _granularity.x;
        memBindInfo.extent.height = mipProp.height - memBindInfo.offset.y < _granularity.y ? mipProp.height - memBindInfo.offset.y : _granularity.y;
        memBindInfo.extent.depth = _granularity.z;
    }

    VkSemaphoreCreateInfo semInfo = {.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    vkCreateSemaphore(_device->device(), &semInfo, nullptr, &bound_semaphore);
    vkCreateSemaphore(_device->device(), &semInfo, nullptr, &submit_semaphore);
}

void SparseImage::bind() {
     auto* q = _device->getQueue({QueueType::SPARSE});
     auto* queue = static_cast<Queue*>(q);
     queue->bindSparse({{this}});
}

void SparseImage::prepare(RHICommandBuffer* cb) {
    auto* cmdBuffer = static_cast<CommandBuffer*>(cb);

    VkImageMemoryBarrier barrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    VkPipelineStageFlags srcStageMask{VK_PIPELINE_STAGE_NONE};
    VkPipelineStageFlags dstStageMask{VK_PIPELINE_STAGE_NONE};

    srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    barrier.image = _sparseImage;
    barrier.srcAccessMask = VK_ACCESS_NONE;
    barrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
    barrier.srcQueueFamilyIndex = _device->getQueue({QueueType::SPARSE})->index();
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.dstQueueFamilyIndex = _device->getQueue({QueueType::SPARSE})->index();

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = _req.imageMipTailFirstLod;

    vkCmdPipelineBarrier(cmdBuffer->commandBuffer(), srcStageMask, dstStageMask, VK_DEPENDENCY_BY_REGION_BIT,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    for (auto [mip, data] : _miptails) {
        uint32_t width = _width >> mip;
        uint32_t height = _height >> mip;
        auto size = getFormatSize(_format) * width * height;
        BufferSourceInfo si{
            .bufferUsage = BufferUsage::TRANSFER_SRC,
            .size = size,
            .data = data,
        };
        auto stagingBuffer = rhi::BufferPtr(_device->createBuffer(si));
        auto* vBuf = static_cast<Buffer*>(stagingBuffer.get());

        ImageBarrierInfo prepareTransfer{
            .sparseImage = this,
            .dstStage = PipelineStage::TRANSFER,
            .oldLayout = ImageLayout::PREINITIALIZED,
            .newLayout = ImageLayout::TRANSFER_DST_OPTIMAL,
            .dstAccessFlag = AccessFlags::TRANSFER_WRITE,
            .range = {
                .sliceCount = 1,
                .firstMip = mip,
                .mipCount = 1,
            },
        };
        cmdBuffer->appendImageBarrier(prepareTransfer);
        cmdBuffer->applyBarrier({DependencyFlags::BY_REGION});

        VkBufferImageCopy region{
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = mip,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
            .imageOffset = {0, 0, 0},
            .imageExtent = {width, height, 1},
        };
        vkCmdCopyBufferToImage(cmdBuffer->commandBuffer(),
                               vBuf->buffer(),
                               _sparseImage,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &region);
        ImageBarrierInfo shaderRead{
            .sparseImage = this,
            .dstStage = PipelineStage::FRAGMENT_SHADER,
            .oldLayout = ImageLayout::TRANSFER_DST_OPTIMAL,
            .newLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL,
            .dstAccessFlag = AccessFlags::SHADER_READ,
            .range = {
                .sliceCount = 1,
                .firstMip = mip,
                .mipCount = 1,
            },
        };
        cmdBuffer->appendImageBarrier(shaderRead);
        cmdBuffer->applyBarrier({DependencyFlags::BY_REGION});

        cmdBuffer->onComplete([stagingBuffer]() mutable {
            stagingBuffer.reset();
        });
    }

    rhi::BufferInfo accessBufferInfo{
        .bufferUsage = rhi::BufferUsage::STORAGE | rhi::BufferUsage::TRANSFER_DST,
        .size = static_cast<uint32_t>(_granularity.x * _granularity.y * sizeof(uint32_t)),
    };
    _accessBuffer = static_cast<Buffer*>(_device->createBuffer(accessBufferInfo));
}

void SparseImage::getRelatedBlocks(uint32_t row, uint32_t col, uint8_t mipIn, std::set<size_t>& indices) {
    int8_t mip = mipIn;
    mip -= 1;
    if (mip < 0) return;
    auto minRow = row * 2;
    auto minCol = col * 2;
    auto maxRow = minRow + 1;
    auto maxCol = minCol + 1;
    for (size_t i = minRow; i <= maxRow && i < _vt.mips[mip].columnCount; ++i) {
        for (size_t j = minCol; j <= maxCol && j < _vt.mips[mip].rowCount; ++j) {
            auto mipProp = _vt.mips[mip];
            auto pageIndex = mipProp.pageStartIndex + j * mipProp.columnCount + i;
            if (!_vt.pageTable[pageIndex].valid) {
                if (mip) {
                    getRelatedBlocks(i, j, mip, indices);
                }
                indices.insert(pageIndex);
            }
        }
    }
}

void SparseImage::analyze(RHIBuffer* buf, RHICommandBuffer* cb) {
    auto* buffer = static_cast<Buffer*>(buf);
    auto vkBuffer = buffer->buffer();
    void* dataBuf;
    vkMapMemory(_device->device(), buffer->allocationInfo().deviceMemory, 0, buffer->info().size, 0, &dataBuf);

    std::queue<PageTable*> purgeQ;
    auto* data = static_cast<uint32_t*>(dataBuf);
    auto width = (_width + _granularity.x - 1) / _granularity.x;
    auto height = (_height + _granularity.y - 1) / _granularity.y;
    for (size_t i = 0; i < width; ++i) {
        for (size_t j = 0; j < height; ++j) {
            auto index = j * width + i;
            if (data[index] >= _req.imageMipTailFirstLod) {
                continue;
            }
            auto x = i >> data[index];
            auto y = j >> data[index];
            auto pageIndex = getPageIndex(x, y, static_cast<uint8_t>(data[index]));
            if (!_vt.pageTable[pageIndex].valid) {
                _vt.update_set.insert(pageIndex);
                getRelatedBlocks(x, y, data[index], _vt.update_set);
            } else {
                int a = 0;
            }
            _vt.pageTable[pageIndex].resident = true;
        }
    }
    vkUnmapMemory(_device->device(), buffer->allocationInfo().deviceMemory);

    for (auto pageIndex : _vt.update_set) {
        auto& page = _vt.pageTable[pageIndex];
        if (page.valid && _vt.sparseImageMemoryBinds[pageIndex].memory) {
            continue;
        }
        page.valid = true;

        page.pageInfo = allocate(pageIndex);
        _vt.sparseImageMemoryBinds[pageIndex].memory = page.pageInfo.memory_sector.memory;
        _vt.sparseImageMemoryBinds[pageIndex].memoryOffset = page.pageInfo.memory_sector.offset;
    }

    //updateAndGenerate(cb);
}

void SparseImage::update(RHICommandBuffer* cb) {
    auto* cmdBuffer = static_cast<CommandBuffer*>(cb);
     updateAndGenerate(cb);
}

void SparseImage::setMiptail(uint8_t* data, uint8_t mip) {
    _miptails.emplace_back(mip, data);
}

void SparseImage::bindSparseImage() {
    // auto* q = _device->getQueue({QueueType::SPARSE});
    // auto* queue = static_cast<Queue*>(q);
    // queue->bindSparse({this});
    // vkQueueWaitIdle(queue->queue());
}

void SparseImage::updateAndGenerate(RHICommandBuffer* cmd) {
    auto* cmdBuffer = static_cast<CommandBuffer*>(cmd);

    //bindSparseImage();
    auto* q = _device->getQueue({QueueType::SPARSE});
    auto* queue = static_cast<Queue*>(q);

    uint32_t level0mipCount = _vt.mips[0].pageCount;

    std::vector<uint8_t> temp_buffer(_vt.pageSize);

    ImageBarrierInfo barrierInfo{};
    barrierInfo.sparseImage = this;

    uint8_t currMip = 0xFF;
    for (auto pageIndex : _vt.update_set) {
        uint8_t mipLevel = getMipLevel(pageIndex);
        ImageSubresourceRange range{};
        range.aspect = AspectMask::COLOR;
        range.firstSlice = 0;
        range.sliceCount = 1;
        range.mipCount = 1;

        if (currMip != mipLevel) {
            if (currMip != 0xFF) {
                range.firstMip = currMip;
                barrierInfo.range = range;
                barrierInfo.srcAccessFlag = AccessFlags::TRANSFER_WRITE;
                barrierInfo.srcStage = PipelineStage::TRANSFER;
                barrierInfo.oldLayout = ImageLayout::TRANSFER_DST_OPTIMAL;
                barrierInfo.dstAccessFlag = AccessFlags::SHADER_READ;
                barrierInfo.dstStage = PipelineStage::FRAGMENT_SHADER;
                barrierInfo.newLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                cmdBuffer->appendImageBarrier(barrierInfo);
                if (currMip != 0) {
                    range.firstMip = currMip - 1;
                    barrierInfo.range = range;
                    barrierInfo.srcAccessFlag = AccessFlags::TRANSFER_READ;
                    barrierInfo.srcStage = PipelineStage::TRANSFER;
                    barrierInfo.oldLayout = ImageLayout::TRANSFER_SRC_OPTIMAL;
                    barrierInfo.dstAccessFlag = AccessFlags::SHADER_READ;
                    barrierInfo.dstStage = PipelineStage::FRAGMENT_SHADER;
                    barrierInfo.newLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                    cmdBuffer->appendImageBarrier(barrierInfo);
                }
            }
            range.firstMip = mipLevel;
            barrierInfo.range = range;
            barrierInfo.srcAccessFlag = AccessFlags::SHADER_READ;
            barrierInfo.srcStage = PipelineStage::FRAGMENT_SHADER;
            barrierInfo.oldLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
            barrierInfo.dstAccessFlag = AccessFlags::TRANSFER_WRITE;
            barrierInfo.dstStage = PipelineStage::TRANSFER;
            barrierInfo.newLayout = ImageLayout::TRANSFER_DST_OPTIMAL;
            cmdBuffer->appendImageBarrier(barrierInfo);
            if (mipLevel != 0) {
                range.firstMip = mipLevel - 1;
                barrierInfo.range = range;
                barrierInfo.srcAccessFlag = AccessFlags::SHADER_READ;
                barrierInfo.srcStage = PipelineStage::FRAGMENT_SHADER;
                barrierInfo.oldLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
                barrierInfo.dstAccessFlag = AccessFlags::TRANSFER_READ;
                barrierInfo.dstStage = PipelineStage::TRANSFER;
                barrierInfo.newLayout = ImageLayout::TRANSFER_SRC_OPTIMAL;
                cmdBuffer->appendImageBarrier(barrierInfo);
            }
            currMip = mipLevel;
        }

        cmdBuffer->applyBarrier({DependencyFlags::BY_REGION});

        VkExtent2D blockExtent;
        blockExtent.width = _vt.sparseImageMemoryBinds[pageIndex].extent.width;
        blockExtent.height = _vt.sparseImageMemoryBinds[pageIndex].extent.height;

        VkOffset2D blockOffset;
        blockOffset.x = _vt.sparseImageMemoryBinds[pageIndex].offset.x;
        blockOffset.y = _vt.sparseImageMemoryBinds[pageIndex].offset.y;

        if (mipLevel == 0) {
            for (size_t row = 0; row < blockExtent.height; ++row) {
                auto formatSize = getFormatSize(_format);
                auto offset = ((row + blockOffset.y) * _width + blockOffset.x) * formatSize;
                memcpy(&temp_buffer[row * blockExtent.width * formatSize], _data + offset, blockExtent.width * formatSize);
            }
            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset.x = blockOffset.x;
            region.imageOffset.y = blockOffset.y;
            region.imageOffset.z = 0;
            region.imageExtent.width = blockExtent.width;
            region.imageExtent.height = blockExtent.height;
            region.imageExtent.depth = 1;

            BufferSourceInfo bufferSourceInfo;
            bufferSourceInfo.data = temp_buffer.data();
            bufferSourceInfo.size = temp_buffer.size();
            bufferSourceInfo.bufferUsage = BufferUsage::TRANSFER_SRC;
            bufferSourceInfo.queueAccess = {queue->index()};

            BufferPtr bufferPtr = BufferPtr(_device->createBuffer(bufferSourceInfo));
            auto* buffer = static_cast<Buffer*>(bufferPtr.get());
            vkCmdCopyBufferToImage(cmdBuffer->commandBuffer(),
                                   buffer->buffer(),
                                   _sparseImage,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1,
                                   &region);
            cmd->onComplete([bufferPtr]() mutable {
                bufferPtr.reset();
            });
        } else {
            VkImageBlit blit;
            blit.srcOffsets[0] = {blockOffset.x * 2, blockOffset.y * 2, 0};
            blit.srcOffsets[1] = {
                static_cast<int32_t>((blockOffset.x + blockExtent.width) * 2),
                static_cast<int32_t>((blockOffset.y + blockExtent.height) * 2),
                1,
            };
            blit.srcSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = static_cast<uint32_t>(mipLevel - 1),
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
            blit.dstOffsets[0] = {blockOffset.x, blockOffset.y, 0};
            blit.dstOffsets[1] = {
                static_cast<int32_t>(blockOffset.x + blockExtent.width),
                static_cast<int32_t>(blockOffset.y + blockExtent.height),
                1,
            };
            blit.dstSubresource = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = mipLevel,
                .baseArrayLayer = 0,
                .layerCount = 1,
            };
            vkCmdBlitImage(cmdBuffer->commandBuffer(),
                           _sparseImage,
                           VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           _sparseImage,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1,
                           &blit,
                           VK_FILTER_LINEAR);
        }
    }

    for (size_t i = 0; i < _vt.pageTable.size(); ++i) {
        auto& pt = _vt.pageTable[i];
        if (!pt.resident && !pt.valid) continue;
        if (!pt.resident && pt.valid) {
            pt.pageInfo.memory_sector.reset();
        }
        pt.valid = pt.resident;
        if (!pt.valid) {
            _vt.sparseImageMemoryBinds[i].memory = nullptr;
            _vt.sparseImageMemoryBinds[i].memoryOffset = 0;
        }
        assert(!(!!_vt.sparseImageMemoryBinds[i].memory ^ pt.valid));
        pt.resident = false;
    }

    _vt.update_set.clear();

    ImageSubresourceRange range{};
    range.aspect = AspectMask::COLOR;
    range.firstSlice = 0;
    range.sliceCount = 1;
    range.mipCount = 1;
    if (currMip != 0xFF) {
        range.firstMip = currMip;
        barrierInfo.range = range;
        barrierInfo.srcAccessFlag = AccessFlags::TRANSFER_WRITE;
        barrierInfo.srcStage = PipelineStage::TRANSFER;
        barrierInfo.oldLayout = ImageLayout::TRANSFER_DST_OPTIMAL;
        barrierInfo.dstAccessFlag = AccessFlags::SHADER_READ;
        barrierInfo.dstStage = PipelineStage::FRAGMENT_SHADER;
        barrierInfo.newLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
        cmdBuffer->appendImageBarrier(barrierInfo);
        if (currMip != 0) {
            range.firstMip = currMip - 1;
            barrierInfo.range = range;
            barrierInfo.srcAccessFlag = AccessFlags::TRANSFER_READ;
            barrierInfo.srcStage = PipelineStage::TRANSFER;
            barrierInfo.oldLayout = ImageLayout::TRANSFER_SRC_OPTIMAL;
            barrierInfo.dstAccessFlag = AccessFlags::SHADER_READ;
            barrierInfo.dstStage = PipelineStage::FRAGMENT_SHADER;
            barrierInfo.newLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
            cmdBuffer->appendImageBarrier(barrierInfo);
        }
        cmdBuffer->applyBarrier({DependencyFlags::BY_REGION});
    }
    for (auto& page : _vt.pageTable) {
        page.needGenMipMap = false;
    }
}

uint32_t SparseImage::getPageIndex(uint32_t row, uint32_t col, uint8_t mip) {
    const auto& mipProp = _vt.mips[mip];
    return mipProp.pageStartIndex + mipProp.columnCount * col + row;
}

uint8_t SparseImage::getMipLevel(uint32_t pageIndex) {
    uint8_t res = 0;
    for (uint8_t i = 0; i < _vt.mips.size(); ++i) {
        if (_vt.mips[i].pageStartIndex + _vt.mips[i].pageCount > pageIndex) {
            res = i;
            break;
        }
    }
    return res;
}

SparseImage::PageInfo SparseImage::allocate(uint32_t pageIndex) {
    PageInfo res;
    if (sectors.empty()) {
        auto& memAllocator = sectors.emplace_back(std::make_shared<MemAllocator>(_device, _memReq));
        res.memory_sector = memAllocator->allocate();
    } else {
        auto it = std::find_if(sectors.begin(), sectors.end(), [](auto& alloc) { return !alloc->full(); });
        if (it == sectors.end()) {
            auto& memAllocator = sectors.emplace_back(std::make_shared<MemAllocator>(_device, _memReq));
            res.memory_sector = memAllocator->allocate();
        } else {
            auto& allocator = *it;
            res.memory_sector = allocator->allocate();
        }
    }
    return res;
}

} // namespace raum::rhi