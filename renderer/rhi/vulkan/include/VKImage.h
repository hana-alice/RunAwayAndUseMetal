#pragma once
#include "RHIImage.h"
#include "vk_mem_alloc.h"
namespace raum::rhi {
class Image {
public:
    explicit Image(const ImageInfo& imgInfo);
    Image() = delete;
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&&) = delete;

    ~Image();
};
} // namespace raum::rhi