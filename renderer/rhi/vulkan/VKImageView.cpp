#include "VKImageView.h"
#include "RHIUtils.h"
#include "VKDevice.h"
#include "VKImage.h"
#include "VKSparseImage.h"
#include "VKUtils.h"
namespace raum::rhi {
ImageView::ImageView(const ImageViewInfo& info, RHIDevice* device)
: RHIImageView(info, device), _device(static_cast<Device*>(device)) {
    VkImage gfxImg{VK_NULL_HANDLE};
    if (isSparse(info.image)) {
        auto* sparseImage = static_cast<SparseImage*>(info.image);
        _image = sparseImage;
        gfxImg = sparseImage->image();
    } else {
        auto* img = static_cast<Image*>(info.image);
        _image = img;
        gfxImg = img->image();
    }
    init(info.type, gfxImg, info.range, info.componentMapping, info.format);
}

void ImageView::init(ImageViewType type, VkImage image, const ImageSubresourceRange& range, ComponentMapping cm, Format format) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image;
    createInfo.viewType = viewType(type);
    createInfo.format = formatInfo(format).format;
    createInfo.components.r = componentSwizzle(cm.r);
    createInfo.components.g = componentSwizzle(cm.g);
    createInfo.components.b = componentSwizzle(cm.b);
    createInfo.components.a = componentSwizzle(cm.a);
    createInfo.subresourceRange.aspectMask = aspectMask(range.aspect);
    createInfo.subresourceRange.baseMipLevel = range.firstMip;
    createInfo.subresourceRange.levelCount = range.mipCount;
    createInfo.subresourceRange.baseArrayLayer = range.firstSlice;
    createInfo.subresourceRange.layerCount = range.sliceCount;

    VkResult res = vkCreateImageView(_device->device(), &createInfo, nullptr, &_imageView);
    raum_check(res == VK_SUCCESS, "failed to create image view");
}

ImageView::~ImageView() {
    vkDestroyImageView(_device->device(), _imageView, nullptr);
}

RHIImage* ImageView::image() const {
    return _image;
}

} // namespace raum::rhi