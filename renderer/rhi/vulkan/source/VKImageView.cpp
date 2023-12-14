#include "VKImageView.h"
#include "VKImage.h"
#include "VKUtils.h"
#include "VKDevice.h"
namespace raum::rhi {
ImageView::ImageView(const ImageViewInfo& info, RHIDevice* device)
    :RHIImageView(info, device), _device(static_cast<Device*>(device)) {
    VkImageViewCreateInfo createInfo{};
    createInfo.image = static_cast<Image*>(info.image)->image();
    createInfo.viewType = viewType(info.type);
    createInfo.format = formatInfo(info.format).format;
    createInfo.components.r = componentSwizzle(info.componentMapping.r);
    createInfo.components.g = componentSwizzle(info.componentMapping.g);
    createInfo.components.b = componentSwizzle(info.componentMapping.b);
    createInfo.components.a = componentSwizzle(info.componentMapping.a);
    createInfo.subresourceRange.aspectMask = aspectMask(info.aspectMask);
    createInfo.subresourceRange.baseMipLevel = info.range.firstMip;
    createInfo.subresourceRange.levelCount = info.range.mipCount;
    createInfo.subresourceRange.baseArrayLayer = info.range.firstSlice;
    createInfo.subresourceRange.layerCount = info.range.sliceCount;

    vkCreateImageView(_device->device(), &createInfo, nullptr, &_imageView);
}

ImageView::~ImageView() {
    vkDestroyImageView(_device->device(), _imageView, nullptr);
}

} // namespace raum::rhi