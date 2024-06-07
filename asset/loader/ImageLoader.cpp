#include "ImageLoader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace raum::asset {

ImageAsset& ImageAsset::operator=(ImageAsset&& rhs) {
    data = rhs.data;
    width = rhs.width;
    height = rhs.height;
    channels = rhs.channels;

    rhs.data = nullptr;
    rhs.width = 0;
    rhs.height = 0;
    rhs.channels = 0;
    return *this;
}

ImageAsset::ImageAsset(ImageAsset&& rhs) {
    data = rhs.data;
    width = rhs.width;
    height = rhs.height;
    channels = rhs.channels;

    rhs = ImageAsset();
}

ImageAsset ImageLoader::load(const std::string_view path) {
    ImageAsset img;
    img.data = stbi_load(path.data(), &img.width, &img.height, &img.channels, 4);
    return img;
}

void ImageLoader::free(ImageAsset&& img) {
    stbi_image_free(img.data);
}

} // namespace raum::framework::asset