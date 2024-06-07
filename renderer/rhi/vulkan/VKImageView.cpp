#include "VKImageView.h"
#include "VKDevice.h"
#include "VKImage.h"
#include "VKUtils.h"
namespace raum::rhi {
ImageView::ImageView(const ImageViewInfo& info, RHIDevice* device)
: RHIImageView(info, device), _device(static_cast<Device*>(device)) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = static_cast<Image*>(info.image)->image();
    createInfo.viewType = viewType(info.type);
    createInfo.format = formatInfo(info.format).format;
    createInfo.components.r = componentSwizzle(info.componentMapping.r);
    createInfo.components.g = componentSwizzle(info.componentMapping.g);
    createInfo.components.b = componentSwizzle(info.componentMapping.b);
    createInfo.components.a = componentSwizzle(info.componentMapping.a);
    createInfo.subresourceRange.aspectMask = aspectMask(info.range.aspect);
    createInfo.subresourceRange.baseMipLevel = info.range.firstMip;
    createInfo.subresourceRange.levelCount = info.range.mipCount;
    createInfo.subresourceRange.baseArrayLayer = info.range.firstSlice;
    createInfo.subresourceRange.layerCount = info.range.sliceCount;

    VkResult res = vkCreateImageView(_device->device(), &createInfo, nullptr, &_imageView);
    raum_check(res == VK_SUCCESS, "failed to create image view");
}

ImageView::~ImageView() {
    vkDestroyImageView(_device->device(), _imageView, nullptr);
}

} // namespace raum::rhi