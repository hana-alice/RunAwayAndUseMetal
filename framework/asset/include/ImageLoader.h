#pragma once
#include <stdint.h>
#include <string>

namespace raum::framework::asset {

struct ImageAsset {
    uint8_t* data{nullptr};
    int32_t width{0};
    int32_t height{0};
    int32_t channels{0};

    ImageAsset() = default;
    ~ImageAsset() = default;
    ImageAsset(ImageAsset&&);
    ImageAsset(const ImageAsset&) = delete;
    ImageAsset& operator=(const ImageAsset&) = delete;
    ImageAsset& operator=(ImageAsset&&);
};

class ImageLoader {
public:
    ImageLoader() = default;
    ~ImageLoader() = default;

    const ImageAsset load(const std::string_view path);
    void free(ImageAsset&&);
};
} // namespace raum::framework::asset