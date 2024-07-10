#pragma once
#include "RHIImageView.h"
#include "VKDefine.h"
#include "VKImageView.h"

namespace raum::rhi {
class Device;
class Swapchain;
class SparseImage;
class Image;
class ImageView : public RHIImageView {
public:
    explicit ImageView(const ImageViewInfo& info, RHIDevice* device);
    explicit ImageView(const SparseImageViewInfo& info, RHIDevice* device);
    ImageView(ImageView&& iv) = delete;
    ImageView() = delete;
    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;

    RHIImage* image() const override;

    ~ImageView();

    VkImageView imageView() const { return _imageView; }

private:
    void init(ImageViewType type, VkImage image, const ImageSubresourceRange& range, ComponentMapping cm, Format format);

    VkImageView _imageView;
    Device* _device{nullptr};
    SparseImage* _sparseImage{nullptr};
    Image* _image{nullptr};

    friend class Swapchain;
};
} // namespace raum::rhi