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
VmaAllocationCreateInfo mapCreateInfo(MemoryUsage usage, bool map = false) {
    VmaAllocationCreateInfo info{};
    switch (usage) {
        case MemoryUsage::HOST_VISIBLE:
            info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
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
    if (map) {
        info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    }
    return info;
}

VkBufferUsageFlags mapBufferUsage(BufferUsage usage) {
    VkBufferUsageFlags flags;
    if (usage & BufferUsage::VERTEX) {
        flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (usage & BufferUsage::INDEX) {
        flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (usage & BufferUsage::UNIFORM) {
        flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (usage & BufferUsage::TRANSFER_SRC) {
        flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (usage & BufferUsage::TRANSFER_DST) {
        flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (usage & BufferUsage::INDIRECT) {
        flags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    return flags;
};
} // namespace

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

    VmaAllocationCreateInfo allocaInfo = mapCreateInfo(info.memUsage, true);

    VmaAllocator& allocator = _device->allocator();

    VkResult res = vmaCreateBuffer(allocator, &bufferInfo, &allocaInfo, &_buffer, &_allocation, nullptr);

    void* mappedData;
    vmaMapMemory(allocator, _allocation, &mappedData);
    memcpy(mappedData, info.data, info.size);
    vmaUnmapMemory(allocator, _allocation);
    vmaFlushAllocation(allocator, _allocation, 0, info.size);
    vmaInvalidateAllocation(allocator, _allocation, 0, info.size);

    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create buffer!");
}
Buffer::Buffer(const BufferInfo& info, RHIDevice* device) : RHIBuffer(info, device), _device(static_cast<Device*>(device)) {
}

Buffer::~Buffer() {
    vmaDestroyBuffer(_device->allocator(), _buffer, _allocation);
}

void Buffer::update(const void* data, uint32_t size, uint32_t offset) {
}

} // namespace raum::rhi