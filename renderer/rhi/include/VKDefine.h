#pragma once
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "define.h"
namespace raum::rhi {

static constexpr uint32_t SwapchainCount{3};

enum class Format {
    // powered by TabNine.
    UNKNOWN,
    A8_UNORM,
    R8_UNORM,
    R8_SNORM,
    R8_UINT,
    R8_SINT,
    R8_SRGB,
    R8G8_UNORM,
    R8G8_SNORM,
    R8G8_UINT,
    R8G8_SINT,
    R8G8_SRGB,
    R8G8B8_UNORM,
    R8G8B8_SNORM,
    R8G8B8_UINT,
    R8G8B8_SINT,
    R8G8B8_SRGB,
    B8G8R8_UNORM,
    B8G8R8_SNORM,
    B8G8R8_UINT,
    B8G8R8_SINT,
    B8G8R8_SRGB,
    R8G8B8A8_UNORM,
    R8G8B8A8_SNORM,
    R8G8B8A8_UINT,
    R8G8B8A8_SINT,
    R8G8B8A8_SRGB,
    B8G8R8A8_UNORM,
    B8G8R8A8_SNORM,
    B8G8R8A8_UINT,
    B8G8R8A8_SINT,
    B8G8R8A8_SRGB,
    A8B8G8R8_UNORM_PACK32,
    A8B8G8R8_SNORM_PACK32,
    A8B8G8R8_UINT_PACK32,
    A8B8G8R8_SINT_PACK32,
    A8B8G8R8_SRGB_PACK32,
    A2R10G10B10_UNORM_PACK32,
    A2R10G10B10_SNORM_PACK32,
    A2R10G10B10_UINT_PACK32,
    A2R10G10B10_SINT_PACK32,
    R16_UNORM,
    R16_SNORM,
    R16_UINT,
    R16_SINT,
    R16_SFLOAT,
    R16G16_UNORM,
    R16G16_SNORM,
    R16G16_UINT,
    R16G16_SINT,
    R16G16_SFLOAT,
    R16G16B16_UNORM,
    R16G16B16_SNORM,
    R16G16B16_UINT,
    R16G16B16_SINT,
    R16G16B16_SFLOAT,
    R16G16B16A16_UNORM,
    R16G16B16A16_SNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_SINT,
    R16G16B16A16_SFLOAT,
    R32_UINT,
    R32_SINT,
    R32_SFLOAT,
    R32G32_UINT,
    R32G32_SINT,
    R32G32_SFLOAT,
    R32G32B32_UINT,
    R32G32B32_SINT,
    R32G32B32_SFLOAT,
    R32G32B32A32_UINT,
    R32G32B32A32_SINT,
    R32G32B32A32_SFLOAT,
    R64_UINT,
    R64_SINT,
    R64_SFLOAT,
    R64G64_UINT,
    R64G64_SINT,
    R64G64_SFLOAT,
    R64G64B64_UINT,
    R64G64B64_SINT,
    R64G64B64_SFLOAT,
    R64G64B64A64_UINT,
    R64G64B64A64_SINT,
    R64G64B64A64_SFLOAT,
    B10G11R11_UFLOAT_PACK32,
    E5B9G9R9_UFLOAT_PACK32,
    D16_UNORM,
    X8_D24_UNORM_PACK32,
    D32_SFLOAT,
    S8_UINT,
    D16_UNORM_S8_UINT,
    D24_UNORM_S8_UINT,
    D32_SFLOAT_S8_UINT,
    BC1_RGB_UNORM_BLOCK,
    BC1_RGB_SRGB_BLOCK,
    BC1_RGBA_UNORM_BLOCK,
    BC1_RGBA_SRGB_BLOCK,
    BC2_UNORM_BLOCK,
    BC2_SRGB_BLOCK,
    BC3_UNORM_BLOCK,
    BC3_SRGB_BLOCK,
    BC4_UNORM_BLOCK,
    BC4_SNORM_BLOCK,
    BC5_UNORM_BLOCK,
    BC5_SNORM_BLOCK,
    BC6H_UFLOAT_BLOCK,
    BC6H_SFLOAT_BLOCK,
    BC7_UNORM_BLOCK,
    BC7_SRGB_BLOCK,
    ETC2_R8G8B8_UNORM_BLOCK,
    ETC2_R8G8B8_SRGB_BLOCK,
    ETC2_R8G8B8A1_UNORM_BLOCK,
    ETC2_R8G8B8A1_SRGB_BLOCK,
    ETC2_R8G8B8A8_UNORM_BLOCK,
    ETC2_R8G8B8A8_SRGB_BLOCK,
    EAC_R11_UNORM_BLOCK,
    EAC_R11_SNORM_BLOCK,
    EAC_R11G11_UNORM_BLOCK,
    EAC_R11G11_SNORM_BLOCK,
    ASTC_4x4_UNORM_BLOCK,
    ASTC_4x4_SRGB_BLOCK,
    ASTC_5x4_UNORM_BLOCK,
    ASTC_5x4_SRGB_BLOCK,
    ASTC_5x5_UNORM_BLOCK,
    ASTC_5x5_SRGB_BLOCK,
    ASTC_6x5_UNORM_BLOCK,
};

enum class QueueType : uint32_t {
    GRAPHICS,
    COMPUTE,
    TRANSFER,
};
struct QueueInfo {
    QueueType type;
};

enum class SyncType : uint32_t {
    IMMEDIATE,
    VSYNC,
    RELAX,
    MAILBOX,
};

struct SwapchainInfo {
    uint32_t width{0};
    uint32_t height{0};
    SyncType type;
    void* hwnd;
};

enum class ShaderStage : uint8_t {
    Vertex,
    Fragment,
    Compute,
};
struct SourceStage {
    ShaderStage stage;
    std::string source;
};
struct BinaryStage {
    ShaderStage stage;
    std::vector<uint32_t> spv;
};

struct ShaderSourceInfo {
    std::string sourcePath{};
    SourceStage stage;
};

struct ShaderBinaryInfo {
    std::string sourcePath{};
    BinaryStage stage;
};

struct ImageInfo {
    Format format{Format::UNKNOWN};
    uint32_t width{0};
    uint32_t height{0};
    uint32_t sliceCount{0};
    uint32_t mipCount{0};
    uint32_t depth{0};
};

struct Range {
    uint32_t width{0};
    uint32_t height{0};
    uint32_t firstSlice{0};
    uint32_t sliceCount{0};
    uint32_t firstMip{0};
    uint32_t mipCount{0};
    uint32_t plane{0};
};

struct ImageViewInfo {
    Range range{};
    Format format{Format::UNKNOWN};
};

struct DeviceInfo {
    // void* hwnd;
};
} // namespace raum::rhi