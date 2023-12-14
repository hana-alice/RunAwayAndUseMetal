#pragma once
#include "RHIImage.h"
#include "vk_mem_alloc.h"
namespace raum::rhi {
class Device;
class Image :public RHIImage {
public:
    explicit Image(const ImageInfo& imgInfo, RHIDevice* device);
    Image() = delete;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;

    ~Image();

    VkImage image() const { return _image; }

private:
    VmaAllocation _allocation;
    VkImage _image;
    Device* _device{nullptr};
};
} // namespace raum::rhi