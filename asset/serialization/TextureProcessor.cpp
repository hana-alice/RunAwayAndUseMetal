#include "TextureProcessor.h"

#include <algorithm>
#include <cmath>
#include "core/utils/log.h"

namespace raum::asset::serialize {

namespace {

float srgbToLinear(float value) {
    return value <= 0.04045f
               ? value / 12.92f
               : std::pow((value + 0.055f) / 1.055f, 2.4f);
}

float linearToSrgb(float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    return value <= 0.0031308f
               ? value * 12.92f
               : 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
}

void preserveAlphaCoverage(std::vector<uint8_t>& pixels,
                           float cutoff,
                           float targetCoverage) {
    if (pixels.empty() || targetCoverage <= 0.0f) {
        return;
    }

    float low = 0.0f;
    float high = 8.0f;
    for (uint32_t iteration = 0; iteration < 16; ++iteration) {
        const float scale = (low + high) * 0.5f;
        if (calculateAlphaCoverage(pixels, cutoff, scale) < targetCoverage) {
            low = scale;
        } else {
            high = scale;
        }
    }

    const float lowError = std::abs(calculateAlphaCoverage(pixels, cutoff, low) - targetCoverage);
    const float highError = std::abs(calculateAlphaCoverage(pixels, cutoff, high) - targetCoverage);
    const float scale = lowError < highError ? low : high;
    for (size_t i = 3; i < pixels.size(); i += 4) {
        pixels[i] = static_cast<uint8_t>(std::clamp(
            std::round(static_cast<float>(pixels[i]) * scale), 0.0f, 255.0f));
    }
}

} // namespace

float calculateAlphaCoverage(std::span<const uint8_t> rgbaPixels,
                             float cutoff,
                             float scale) {
    if (rgbaPixels.empty()) {
        return 0.0f;
    }
    if (rgbaPixels.size() % 4 != 0) {
        raum_error("RGBA texture data size is not divisible by four");
    }

    const auto alphaCutoff = cutoff * 255.0f;
    size_t covered{0};
    for (size_t i = 3; i < rgbaPixels.size(); i += 4) {
        covered += std::min(static_cast<float>(rgbaPixels[i]) * scale, 255.0f) >= alphaCutoff;
    }
    return static_cast<float>(covered) / static_cast<float>(rgbaPixels.size() / 4);
}

TextureMipChain buildTextureMipChain(uint32_t width,
                                     uint32_t height,
                                     std::span<const uint8_t> rgbaPixels,
                                     const TextureMipBuildInfo& info) {
    if (!width || !height || rgbaPixels.size() != static_cast<size_t>(width) * height * 4) {
        raum_error("Invalid RGBA texture dimensions or data size");
    }

    TextureMipChain chain;
    chain.mipCount = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    std::vector<uint8_t> previous(rgbaPixels.begin(), rgbaPixels.end());
    const float targetCoverage = info.alphaCutoff
                                     ? calculateAlphaCoverage(previous, *info.alphaCutoff)
                                     : 0.0f;
    uint32_t mipWidth = width;
    uint32_t mipHeight = height;

    for (uint32_t mip = 0; mip < chain.mipCount; ++mip) {
        chain.pixels.insert(chain.pixels.end(), previous.begin(), previous.end());
        if (mip + 1 == chain.mipCount) {
            break;
        }

        const uint32_t nextWidth = std::max(1U, mipWidth / 2);
        const uint32_t nextHeight = std::max(1U, mipHeight / 2);
        std::vector<uint8_t> next(static_cast<size_t>(nextWidth) * nextHeight * 4);
        for (uint32_t y = 0; y < nextHeight; ++y) {
            for (uint32_t x = 0; x < nextWidth; ++x) {
                const uint32_t beginX = x * mipWidth / nextWidth;
                const uint32_t endX = std::max(beginX + 1, (x + 1) * mipWidth / nextWidth);
                const uint32_t beginY = y * mipHeight / nextHeight;
                const uint32_t endY = std::max(beginY + 1, (y + 1) * mipHeight / nextHeight);
                float channels[4]{};
                uint32_t samples{0};
                for (uint32_t sourceY = beginY; sourceY < endY; ++sourceY) {
                    for (uint32_t sourceX = beginX; sourceX < endX; ++sourceX) {
                        const size_t source = (static_cast<size_t>(sourceY) * mipWidth + sourceX) * 4;
                        for (uint32_t channel = 0; channel < 3; ++channel) {
                            const float value = static_cast<float>(previous[source + channel]) / 255.0f;
                            channels[channel] += info.srgbColor ? srgbToLinear(value) : value;
                        }
                        channels[3] += static_cast<float>(previous[source + 3]) / 255.0f;
                        ++samples;
                    }
                }

                const size_t destination = (static_cast<size_t>(y) * nextWidth + x) * 4;
                for (uint32_t channel = 0; channel < 3; ++channel) {
                    float value = channels[channel] / static_cast<float>(samples);
                    if (info.srgbColor) {
                        value = linearToSrgb(value);
                    }
                    next[destination + channel] = static_cast<uint8_t>(
                        std::clamp(std::round(value * 255.0f), 0.0f, 255.0f));
                }
                next[destination + 3] = static_cast<uint8_t>(std::clamp(
                    std::round(channels[3] / static_cast<float>(samples) * 255.0f),
                    0.0f,
                    255.0f));
            }
        }

        if (info.alphaCutoff) {
            preserveAlphaCoverage(next, *info.alphaCutoff, targetCoverage);
        }
        previous = std::move(next);
        mipWidth = nextWidth;
        mipHeight = nextHeight;
    }
    return chain;
}

} // namespace raum::asset::serialize
