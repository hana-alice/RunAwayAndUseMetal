#pragma once
#include "VKDefine.h"

namespace raum::rhi {
class Image;
class Swapchain;
class ImageView {
public:
    explicit ImageView(const ImageViewInfo& info, const Image* image);
    explicit ImageView(const ImageViewInfo& info, const Swapchain* swapchain, uint32_t index);
    ImageView(ImageView&& iv) = delete;
    ImageView() = delete;
    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    ~ImageView();
};
} // namespace raum::rhi