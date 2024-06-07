#include "VKImage.h"
#include "VKUtils.h"
#include "VKDevice.h"
namespace raum::rhi {

Image::Image(const ImageInfo& info, RHIDevice* device)
: RHIImage(info, device), _device(static_cast<Device*>(device)) {
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.format = formatInfo(info.format).format;
    createInfo.imageType = imageType(info.type);
    if (info.imageFlag != ImageFlag::NONE) {
        createInfo.flags = imageFlag(info.imageFlag);
    }
    createInfo.extent.width = info.extent.x;
    createInfo.extent.height = info.extent.y;
    createInfo.extent.depth = info.extent.z;
    createInfo.mipLevels = info.mipCount;
    createInfo.arrayLayers = info.sliceCount;
    createInfo.samples = sampleCount(info.sampleCount);
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = imageUsage(info.usage);
    createInfo.sharingMode = sharingMode(info.shareingMode);
    if (!info.queueAccess.empty()) {
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(info.queueAccess.size());
        createInfo.pQueueFamilyIndices = info.queueAccess.data();
    }
    createInfo.initialLayout = imageLayout(info.intialLayout);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocInfo.priority = 1.0f;

    VkResult res = vmaCreateImage(_device->allocator(), &createInfo, &allocInfo, &_image, &_allocation, nullptr);
    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create image.");
}

Image::Image(const ImageInfo& imgInfo, RHIDevice* device, VkImage image)
: RHIImage(imgInfo, device), _swapchain(true), _image(image) {
}

Image::~Image() {
    if (!_swapchain) {
        vmaDestroyImage(_device->allocator(), _image, _allocation);
    }
}

}