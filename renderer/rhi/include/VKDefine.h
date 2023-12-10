#pragma once
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "define.h"
namespace raum::rhi {

static constexpr uint32_t SwapchainCount{3};

enum class Format : uint16_t {
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
    ASTC_4x4_UNORM_BLOCK,
    ASTC_4x4_SRGB_BLOCK,
    ASTC_5x4_UNORM_BLOCK,
    ASTC_5x4_SRGB_BLOCK,
    ASTC_5x5_UNORM_BLOCK,
    ASTC_5x5_SRGB_BLOCK,
    ASTC_6x5_UNORM_BLOCK,
};

struct FormatInfo {
    VkFormat format;
    uint32_t size;
    uint32_t macroPixelCount;
};

enum class QueueType : uint8_t {
    GRAPHICS,
    COMPUTE,
    TRANSFER,
};
struct QueueInfo {
    QueueType type;
};

enum class SyncType : uint8_t {
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
    VERTEX,
    TASK,
    MESH,
    FRAGMENT,
    COMPUTE,
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

enum class InputRate : uint8_t {
    PER_VERTEX,
    PER_INSTANCE,
};

struct VertexAttribute {
    uint32_t location{0};
    uint32_t binding{0};
    Format format;
    InputRate rate;
};

using VertexBufferAttribute = std::vector<VertexAttribute>;

using VertexLayout = std::vector<VertexBufferAttribute>;
struct FragmentLayout {
};

// struct GraphicsPipelineLayout {
//     VertexLayout vertexLayout;
// };

class Shader;
struct GraphicsPipelineStateInfo {
    std::vector<Shader*> shaders;
    VertexLayout vertexLayout;
};

enum class MemoryUsage : uint8_t {
    HOST_VISIBLE,
    DEVICE_ONLY,
    STAGING,
    LAZY_ALLOCATED,
};

enum class BufferUsage : uint8_t {
    UNIFORM = 1,
    STORAGE = 1 << 1,
    INDEX = 1 << 2,
    VERTEX = 1 << 3,
    INDIRECT = 1 << 4,
    TRANSFER_SRC = 1 << 5,
    TRANSFER_DST = 1 << 6,
};

struct BufferSourceInfo {
    const uint8_t* data{nullptr};
    uint32_t size{0};
    MemoryUsage memUsage{MemoryUsage::DEVICE_ONLY};
    BufferUsage bufferUsage{BufferUsage::UNIFORM};
};

struct BufferInfo {
    uint32_t size{0};
    MemoryUsage memUsage{MemoryUsage::DEVICE_ONLY};
    BufferUsage bufferUsage{BufferUsage::UNIFORM};
};

enum class IndexType : uint8_t {
    HALF, // most likely 16 bit
    FULL, // most likely 32 bit
};

struct DeviceInfo {
    // void* hwnd;
};
} // namespace raum::rhi