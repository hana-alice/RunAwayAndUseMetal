#pragma once
#include "VKImageView.h"
#include "VKDefine.h"
#include "RHIImageView.h"

namespace raum::rhi {
class Device;
class ImageView :public RHIImageView {
public:
    explicit ImageView(const ImageViewInfo& info, RHIDevice* device);
    ImageView(ImageView&& iv) = delete;
    ImageView() = delete;
    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    ~ImageView();

    VkImageView imageView() const { return _imageView; }

private:
    VkImageView _imageView;
    Device* _device{nullptr};
};
} // namespace raum::rhi