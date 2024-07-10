#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKUtils.h"

namespace raum::rhi {

namespace {
std::underlying_type<BufferUsage>::type operator&(BufferUsage lhs, BufferUsage rhs) {
    return static_cast<std::underlying_type<BufferUsage>::type>(lhs) & static_cast<std::underlying_type<BufferUsage>::type>(rhs);
}

std::underlying_type<MemoryUsage>::type operator&(MemoryUsage lhs, MemoryUsage rhs) {
    return static_cast<std::underlying_type<MemoryUsage>::type>(lhs) & static_cast<std::underlying_type<MemoryUsage>::type>(rhs);
}
// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
VmaAllocationCreateInfo mapCreateInfo(MemoryUsage usage) {
    VmaAllocationCreateInfo info{};
    switch (usage) {
        case MemoryUsage::HOST_VISIBLE:
            info.usage = VMA_MEMORY_USAGE_AUTO;
            info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
            break;
        case MemoryUsage::DEVICE_ONLY:
            info.usage = VMA_MEMORY_USAGE_AUTO;
            info.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            break;
        case MemoryUsage::STAGING:
            info.usage = VMA_MEMORY_USAGE_AUTO;
            info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
            break;
        case MemoryUsage::LAZY_ALLOCATED:
            info.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
            break;
    };
    return info;
}

VkBufferUsageFlags mapBufferUsage(BufferUsage usage) {
    VkBufferUsageFlags flags{0};
    if (test(usage, BufferUsage::VERTEX)) {
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (test(usage, BufferUsage::INDEX)) {
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (test(usage, BufferUsage::UNIFORM)) {
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (test(usage, BufferUsage::TRANSFER_SRC)) {
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (test(usage, BufferUsage::TRANSFER_DST)) {
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (test(usage, BufferUsage::INDIRECT)) {
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    if(test(usage, BufferUsage::STORAGE)) {
        flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    return flags;
};
} // namespace

Buffer::Buffer(const BufferInfo& info, RHIDevice* device) : RHIBuffer(info, device), _device(static_cast<Device*>(device)) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = info.size;
    bufferInfo.usage = mapBufferUsage(info.bufferUsage);
    bufferInfo.sharingMode = sharingMode(info.sharingMode);
    if (info.flag != BufferFlag::NONE) {
        bufferInfo.flags = bufferFlag(info.flag);
    }
    if (!info.queueAccess.empty()) {
        bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(info.queueAccess.size());
        bufferInfo.pQueueFamilyIndices = info.queueAccess.data();
    }

    VmaAllocationCreateInfo allocaInfo = mapCreateInfo(info.memUsage);

    VmaAllocator& allocator = _device->allocator();

    VkResult res = vmaCreateBuffer(allocator, &bufferInfo, &allocaInfo, &_buffer, &_allocation, &_allocInfo);
    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create buffer!");
}

Buffer::Buffer(const BufferSourceInfo& info, RHIDevice* device) : RHIBuffer(info, device), _device(static_cast<Device*>(device)) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = info.size;
    bufferInfo.usage = mapBufferUsage(info.bufferUsage);
    bufferInfo.sharingMode = sharingMode(info.sharingMode);
    if (info.flag != BufferFlag::NONE) {
        bufferInfo.flags = bufferFlag(info.flag);
    }
    if (!info.queueAccess.empty()) {
        bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(info.queueAccess.size());
        bufferInfo.pQueueFamilyIndices = info.queueAccess.data();
    }

    VmaAllocationCreateInfo allocaInfo = mapCreateInfo(MemoryUsage::HOST_VISIBLE);
    allocaInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocator& allocator = _device->allocator();

    VkResult res = vmaCreateBuffer(allocator, &bufferInfo, &allocaInfo, &_buffer, &_allocation, &_allocInfo);
    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create buffer!");

    memcpy(_allocInfo.pMappedData, info.data, info.size);
}

Buffer::~Buffer() {
    vmaDestroyBuffer(_device->allocator(), _buffer, _allocation);
}

StagingBuffer::StagingBuffer(VmaAllocator allocator) : _allocator(allocator) {
    auto& chunk = _chunks.emplace_back();
    chunk.size = CHUNK_SIZE;

    VkBufferCreateInfo bufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufCreateInfo.size = CHUNK_SIZE;
    bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                            VMA_ALLOCATION_CREATE_MAPPED_BIT;

    VmaAllocationInfo allocInfo;
    vmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &chunk.buffer, &chunk.allocation, &allocInfo);
}

StagingInfo StagingBuffer::alloc(uint32_t size) {
    auto* curChunk = &_chunks.back();
    if (size + curChunk->offset > curChunk->size) {
        curChunk = &_chunks.emplace_back();
        VkBufferCreateInfo bufCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufCreateInfo.size = CHUNK_SIZE;
        bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        VmaAllocationCreateInfo allocCreateInfo = {};
        allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                VMA_ALLOCATION_CREATE_MAPPED_BIT;

        VmaAllocationInfo allocInfo;
        vmaCreateBuffer(_allocator, &bufCreateInfo, &allocCreateInfo, &curChunk->buffer, &curChunk->allocation, &allocInfo);

    }
    StagingInfo info;
    info.buffer = curChunk->buffer;
    info.offset = curChunk->offset;
    curChunk->offset += size;
    return info;
}

void StagingBuffer::reset() {
    for (auto& chunk : _chunks) {
        chunk.offset = 0;
    }
}

StagingBuffer::~StagingBuffer() {
    for (auto& chunk : _chunks) {
        vmaDestroyBuffer(_allocator, chunk.buffer, chunk.allocation);
    }
}

} // namespace raum::rhi