#include "VKSparseImage.h"
#include "RHIUtils.h"
#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKQueue.h"
#include "VKUtils.h"

namespace raum::rhi {

SparseImage::SparseImage(const SparseImageInfo& info, Device* dev)
: RHISparseImage(info, dev), _device(static_cast<Device*>(dev)){
    uint32_t width = info.width;
    uint32_t height = info.height;

    _info.mipCount = info.maxMip;

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
    createInfo.mipLevels = info.maxMip;
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
    _memTypeIndex = typeIndex;

    _granularity = {
        reqs[0].formatProperties.imageGranularity.width,
        reqs[0].formatProperties.imageGranularity.height,
        reqs[0].formatProperties.imageGranularity.depth,
    };
}

uint8_t SparseImage::firstMipTail() {
    return _req.imageMipTailFirstLod;
}

void SparseImage::bind(SparseType type) {
    // if (_vt.update_set.empty()) return;
    auto* q = _device->getQueue({QueueType::SPARSE});
    auto* queue = static_cast<Queue*>(q);
    queue->bindSparse(SparseBindingInfo{{this}}, type);
}

void SparseImage::prepareMiptail(RHICommandBuffer* cb) {
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
            .image = this,
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
            .image = this,
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
}

void SparseImage::prepare(RHICommandBuffer* cmdBuffer, uint32_t numCols, uint32_t numRows, uint32_t pageCount, uint32_t pageSize) {
    // bind tail
    auto miptailSize = _req.imageMipTailSize;
    VkMemoryRequirements memrequires{
        .size = miptailSize,
        .alignment = _memReq.alignment,
        .memoryTypeBits = _memReq.memoryTypeBits,
    };
    VmaAllocationInfo ai;
    VmaAllocationCreateInfo aci{};
    aci.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    aci.memoryTypeBits = memrequires.memoryTypeBits;
    auto res = vmaAllocateMemoryPages(_device->allocator(), &memrequires, &aci, 1, &_miptailAlloc, &ai);
    assert(res == VK_SUCCESS);

    _miptailBind = {};
    _miptailBind.size = miptailSize;
    _miptailBind.resourceOffset = _req.imageMipTailOffset;
    _miptailBind.memory = ai.deviceMemory;
    _miptailBind.memoryOffset = ai.offset;

    bind(SparseType::OPAQUE);
    prepareMiptail(cmdBuffer);
}

void SparseImage::analyze(RHIBuffer* buf, RHICommandBuffer* cb) {
}

void SparseImage::allocatePage(uint32_t pageIndex) {
    auto page = allocate(pageIndex);
    _pages[pageIndex] = page;
    _imageMemoryBinds[pageIndex].memory = page.memory_sector.memory;
    _imageMemoryBinds[pageIndex].memoryOffset = page.memory_sector.offset;
}

void SparseImage::update(RHICommandBuffer* cb) {
}

void SparseImage::setMiptail(uint8_t* data, uint8_t mip) {
    _miptails.emplace_back(mip, data);
}

void SparseImage::shrink() {
}

const std::vector<VkSparseImageMemoryBind>& SparseImage::sparseImageMemoryBinds() {
    return _imageMemoryBinds;
}

SparseImage::PageInfo SparseImage::allocate(uint32_t pageIndex) {
    PageInfo res;
    if (sectors.empty()) {
        auto& memAllocator = sectors.emplace_back(std::make_shared<MemAllocator>(_device, _memReq, _pageSize));
        res.memory_sector = memAllocator->allocate();
    } else {
        auto it = std::find_if(sectors.begin(), sectors.end(), [](auto& alloc) { return !alloc->full(); });
        if (it == sectors.end()) {
            auto& memAllocator = sectors.emplace_back(std::make_shared<MemAllocator>(_device, _memReq, _pageSize));
            res.memory_sector = memAllocator->allocate();
        } else {
            auto& allocator = *it;
            res.memory_sector = allocator->allocate();
        }
    }
    return res;
}

void SparseImage::reset(uint32_t pageIndex) {
    _pages[pageIndex].memory_sector.reset();
    _imageMemoryBinds[pageIndex].memory = VK_NULL_HANDLE;
    _imageMemoryBinds[pageIndex].memoryOffset = 0;
}

void SparseImage::setPageMemoryBindInfo(uint32_t pageIndex, const Vec3u& offset, const Vec3u& extent, uint8_t mip, uint32_t slice) {
    _imageMemoryBinds[pageIndex].offset.x = offset.x;
    _imageMemoryBinds[pageIndex].offset.y = offset.y;
    _imageMemoryBinds[pageIndex].offset.z = offset.z;

    _imageMemoryBinds[pageIndex].extent = {extent.x, extent.y, extent.z};

    _imageMemoryBinds[pageIndex].subresource.arrayLayer = slice;
    _imageMemoryBinds[pageIndex].subresource.mipLevel = mip;
    _imageMemoryBinds[pageIndex].subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
}

void SparseImage::initPageInfo(uint32_t pageCount, uint32_t pageSize) {
    _imageMemoryBinds.resize(pageCount, {});
    _pages.resize(pageCount, {});

    _pageSize = pageSize;
}

SparseImage::~SparseImage() {
    vkDestroyImage(_device->device(), _sparseImage, nullptr);
    vmaFreeMemoryPages(_device->allocator(), 1, &_miptailAlloc);
}

} // namespace raum::rhi