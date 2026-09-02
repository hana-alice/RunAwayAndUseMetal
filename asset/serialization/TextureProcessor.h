#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace raum::asset::serialize {

struct TextureMipBuildInfo {
    bool srgbColor{false};
    std::optional<float> alphaCutoff;
};

struct TextureMipChain {
    uint32_t mipCount{1};
    std::vector<uint8_t> pixels;
};

float calculateAlphaCoverage(std::span<const uint8_t> rgbaPixels,
                             float cutoff,
                             float scale = 1.0f);

TextureMipChain buildTextureMipChain(uint32_t width,
                                     uint32_t height,
                                     std::span<const uint8_t> rgbaPixels,
                                     const TextureMipBuildInfo& info);

} // namespace raum::asset::serialize
