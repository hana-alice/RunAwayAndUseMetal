#pragma once
#include <map>
#include "RHIDefine.h"
#include "RHIDescriptorSetLayout.h"
namespace raum::rhi {

inline const DescriptorPoolInfo makeDescriptorPoolInfo(const std::vector<RHIDescriptorSetLayout*>& layouts) {
    DescriptorPoolInfo info{};
    info.maxSets = static_cast<uint32_t>(layouts.size());
    std::map<DescriptorType, uint32_t> dict;
    for (auto* layout : layouts) {
        for (const auto& bindingInfo : layout->info().descriptorBindings) {
            ++dict[bindingInfo.type];
        }
    }
    std::vector<DescriptorPoolSize> poolSize;
    poolSize.reserve(dict.size());
    for (const auto& [type, count] : dict) {
        poolSize.emplace_back(DescriptorPoolSize{type, count});
    }
    info.pools = std::move(poolSize);
    return info;
}

template <IndexType SIZE>
struct NumericIndexType;

template<>
struct NumericIndexType<IndexType::FULL> { using type = uint32_t; };

template<>
struct NumericIndexType<IndexType::HALF> { using type = uint16_t; };

using HalfIndexType = NumericIndexType<IndexType::HALF>::type;
using FullIndexType = NumericIndexType<IndexType::FULL>::type;


static constexpr std::array<uint32_t, static_cast<size_t>(DataType::COUNT)> typeSize = {
    0,  // UNKNOWN
    4,  // BOOL
    8,  // BOOL2
    12, // BOOL3
    16, // BOOL4
    4,  // INT
    8,  // INT2
    12, // INT3
    16, // INT4
    4,  // UINT
    8,  // UINT2
    12, // UINT3
    16, // UINT4
    4,  // FLOAT
    8,  // FLOAT2
    12, // FLOAT3
    16, // FLOAT4
    16, // MAT2
    24, // MAT2X3
    32, // MAT2X4
    24, // MAT3X2
    36, // MAT3
    48, // MAT3X4
    32, // MAT4X2
    48, // MAT4X3
    64, // MAT4
};

static const std::unordered_map<DataType, std::string_view> type2str{
    {DataType::UNKNOWN, "unknown"},
    {DataType::BOOL, "bool"},
    {DataType::BOOL2, "bool2"},
    {DataType::BOOL3, "bool3"},
    {DataType::BOOL4, "bool4"},
    {DataType::INT, "int"},
    {DataType::INT2, "int2"},
    {DataType::INT3, "int3"},
    {DataType::INT4, "int4"},
    {DataType::UINT, "uint"},
    {DataType::UINT2, "uint2"},
    {DataType::UINT3, "uint3"},
    {DataType::UINT4, "uint4"},
    {DataType::FLOAT, "float"},
    {DataType::FLOAT2, "float2"},
    {DataType::FLOAT3, "float3"},
    {DataType::FLOAT4, "float4"},
    {DataType::MAT2, "mat2"},
    {DataType::MAT2X3, "mat2x3"},
    {DataType::MAT2X4, "mat2x4"},
    {DataType::MAT3X2, "mat3x2"},
    {DataType::MAT3, "mat3"},
    {DataType::MAT3X4, "mat3x4"},
    {DataType::MAT4X2, "mat4x2"},
    {DataType::MAT3X4, "mat3x4"},
    {DataType::MAT4X3, "mat4x3"},
    {DataType::MAT4, "mat4"},
};

static const std::unordered_map<std::string_view, DataType, hash_string, std::equal_to<>> str2type{
    {"unknown", DataType::UNKNOWN},
    {"bool", DataType::BOOL},
    {"bool2", DataType::BOOL2},
    {"bool3", DataType::BOOL3},
    {"bool4", DataType::BOOL4},
    {"int", DataType::INT},
    {"int2", DataType::INT2},
    {"int3", DataType::INT3},
    {"int4", DataType::INT4},
    {"uint", DataType::UINT},
    {"uint2", DataType::UINT2},
    {"uint3", DataType::UINT3},
    {"uint4", DataType::UINT4},
    {"float", DataType::FLOAT},
    {"float2", DataType::FLOAT2},
    {"float3", DataType::FLOAT3},
    {"float4", DataType::FLOAT4},
    {"mat2", DataType::MAT2},
    {"mat2x3", DataType::MAT2X3},
    {"mat2x4", DataType::MAT2X4},
    {"mat3x2", DataType::MAT3X2},
    {"mat3", DataType::MAT3},
    {"mat3x4", DataType::MAT3X4},
    {"mat4x2", DataType::MAT4X2},
    {"mat3x4", DataType::MAT3X4},
    {"mat4x3", DataType::MAT4X3},
    {"mat4", DataType::MAT4},
};

static const std::unordered_map<std::string_view, Format, hash_string, std::equal_to<>> str2format{
    {"unknown", Format::UNKNOWN},
    {"a8_unorm", Format::A8_UNORM},
    {"r8_unorm", Format::R8_UNORM},
    {"r8_snorm", Format::R8_SNORM},
    {"r8_uint", Format::R8_UINT},
    {"r8_sint", Format::R8_SINT},
    {"r8_srgb", Format::R8_SRGB},
    {"rg8_unorm", Format::RG8_UNORM},
    {"rg8_snorm", Format::RG8_SNORM},
    {"rg8_uint", Format::RG8_UINT},
    {"rg8_sint", Format::RG8_SINT},
    {"rg8_srgb", Format::RG8_SRGB},
    {"rgb8_unorm", Format::RGB8_UNORM},
    {"rgb8_snorm", Format::RGB8_SNORM},
    {"rgb8_uint", Format::RGB8_UINT},
    {"rgb8_sint", Format::RGB8_SINT},
    {"rgb8_srgb", Format::RGB8_SRGB},
    {"bgr8_unorm", Format::BGR8_UNORM},
    {"bgr8_snorm", Format::BGR8_SNORM},
    {"bgr8_uint", Format::BGR8_UINT},
    {"bgr8_sint", Format::BGR8_SINT},
    {"bgr8_srgb", Format::BGR8_SRGB},
    {"rgba8_unorm", Format::RGBA8_UNORM},
    {"rgba8_snorm", Format::RGBA8_SNORM},
    {"rgba8_uint", Format::RGBA8_UINT},
    {"rgba8_sint", Format::RGBA8_SINT},
    {"rgba8_srgb", Format::RGBA8_SRGB},
    {"bgra8_unorm", Format::BGRA8_UNORM},
    {"bgra8_snorm", Format::BGRA8_SNORM},
    {"bgra8_uint", Format::BGRA8_UINT},
    {"bgra8_sint", Format::BGRA8_SINT},
    {"bgra8_srgb", Format::BGRA8_SRGB},
    {"r16_unorm", Format::R16_UNORM},
    {"r16_snorm", Format::R16_SNORM},
    {"r16_uint", Format::R16_UINT},
    {"r16_sint", Format::R16_SINT},
    {"r16_sfloat", Format::R16_SFLOAT},
    {"rg16_unorm", Format::RG16_UNORM},
    {"rg16_snorm", Format::RG16_SNORM},
    {"rg16_uint", Format::RG16_UINT},
    {"rg16_sint", Format::RG16_SINT},
    {"rg16_sfloat", Format::RG16_SFLOAT},
    {"rgb16_unorm", Format::RGB16_UNORM},
    {"rgb16_snorm", Format::RGB16_SNORM},
    {"rgb16_uint", Format::RGB16_UINT},
    {"rgb16_sint", Format::RGB16_SINT},
    {"rgb16_sfloat", Format::RGB16_SFLOAT},
    {"rgba16_unorm", Format::RGBA16_UNORM},
    {"rgba16_snorm", Format::RGBA16_SNORM},
    {"rgba16_uint", Format::RGBA16_UINT},
    {"rgba16_sint", Format::RGBA16_SINT},
    {"rgba16_sfloat", Format::RGBA16_SFLOAT},
    {"r32_uint", Format::R32_UINT},
    {"r32_sint", Format::R32_SINT},
    {"r32_sfloat", Format::R32_SFLOAT},
    {"rg32_uint", Format::RG32_UINT},
    {"rg32_sint", Format::RG32_SINT},
    {"rg32_sfloat", Format::RG32_SFLOAT},
    {"rgb32_uint", Format::RGB32_UINT},
    {"rgb32_sint", Format::RGB32_SINT},
    {"rgb32_sfloat", Format::RGB32_SFLOAT},
    {"rgba32_uint", Format::RGBA32_UINT},
    {"rgba32_sint", Format::RGBA32_SINT},
    {"rgba32_sfloat", Format::RGBA32_SFLOAT},
    {"r64_uint", Format::R64_UINT},
    {"r64_sint", Format::R64_SINT},
    {"r64_sfloat", Format::R64_SFLOAT},
    {"rg64_uint", Format::RG64_UINT},
    {"rg64_sint", Format::RG64_SINT},
    {"rg64_sfloat", Format::RG64_SFLOAT},
    {"rgb64_uint", Format::RGB64_UINT},
    {"rgb64_sint", Format::RGB64_SINT},
    {"rgb64_sfloat", Format::RGB64_SFLOAT},
    {"rgba64_uint", Format::RGBA64_UINT},
    {"rgba64_sint", Format::RGBA64_SINT},
    {"rgba64_sfloat", Format::RGBA64_SFLOAT},
    {"d16_unorm", Format::D16_UNORM},
    {"x8_d24_unorm_pack32", Format::X8_D24_UNORM_PACK32},
    {"d32_sfloat", Format::D32_SFLOAT},
    {"s8_uint", Format::S8_UINT},
    {"d16_unorm_s8_uint", Format::D16_UNORM_S8_UINT},
    {"d24_unorm_s8_uint", Format::D24_UNORM_S8_UINT},
    {"d32_sfloat_s8_uint", Format::D32_SFLOAT_S8_UINT},
    {"bc1_rgb_unorm", Format::BC1_RGB_UNORM},
    {"bc1_rgb_srgb", Format::BC1_RGB_SRGB},
    {"bc1_rgba_unorm", Format::BC1_RGBA_UNORM},
    {"bc1_rgba_srgb", Format::BC1_RGBA_SRGB},
    {"bc2_unorm", Format::BC2_UNORM},
    {"bc2_srgb", Format::BC2_SRGB},
    {"bc3_unorm", Format::BC3_UNORM},
    {"bc3_srgb", Format::BC3_SRGB},
    {"bc4_unorm", Format::BC4_UNORM},
    {"bc4_snorm", Format::BC4_SNORM},
    {"bc5_unorm", Format::BC5_UNORM},
    {"bc5_snorm", Format::BC5_SNORM},
    {"bc6h_ufloat", Format::BC6H_UFLOAT},
    {"bc6h_sfloat", Format::BC6H_SFLOAT},
    {"bc7_unorm", Format::BC7_UNORM},
    {"bc7_srgb", Format::BC7_SRGB},
    {"etc2_rgb8_unorm", Format::ETC2_RGB8_UNORM},
    {"etc2_rgb8_srgb", Format::ETC2_RGB8_SRGB},
    {"etc2_rgb8a1_unorm", Format::ETC2_RGB8A1_UNORM},
    {"etc2_rgb8a1_srgb", Format::ETC2_RGB8A1_SRGB},
    {"etc2_rgba8_unorm", Format::ETC2_RGBA8_UNORM},
    {"etc2_rgba8_srgb", Format::ETC2_RGBA8_SRGB},
    {"astc_4x4_unorm", Format::ASTC_4x4_UNORM},
    {"astc_4x4_srgb", Format::ASTC_4x4_SRGB},
    {"astc_5x4_unorm", Format::ASTC_5x4_UNORM},
    {"astc_5x4_srgb", Format::ASTC_5x4_SRGB},
    {"astc_5x5_unorm", Format::ASTC_5x5_UNORM},
    {"astc_5x5_srgb", Format::ASTC_5x5_SRGB},
    {"astc_6x5_unorm", Format::ASTC_6x5_UNORM},
};

static const std::unordered_map<Format, std::string_view> format2str{
    {Format::UNKNOWN, "unknown"},
    {Format::A8_UNORM, "a8_unorm"},
    {Format::R8_UNORM, "r8_unorm"},
    {Format::R8_SNORM, "r8_snorm"},
    {Format::R8_UINT, "r8_uint"},
    {Format::R8_SINT, "r8_sint"},
    {Format::R8_SRGB, "r8_srgb"},
    {Format::RG8_UNORM, "rg8_unorm"},
    {Format::RG8_SNORM, "rg8_snorm"},
    {Format::RG8_UINT, "rg8_uint"},
    {Format::RG8_SINT, "rg8_sint"},
    {Format::RG8_SRGB, "rg8_srgb"},
    {Format::RGB8_UNORM, "rgb8_unorm"},
    {Format::RGB8_SNORM, "rgb8_snorm"},
    {Format::RGB8_UINT, "rgb8_uint"},
    {Format::RGB8_SINT, "rgb8_sint"},
    {Format::RGB8_SRGB, "rgb8_srgb"},
    {Format::BGR8_UNORM, "bgr8_unorm"},
    {Format::BGR8_SNORM, "bgr8_snorm"},
    {Format::BGR8_UINT, "bgr8_uint"},
    {Format::BGR8_SINT, "bgr8_sint"},
    {Format::BGR8_SRGB, "bgr8_srgb"},
    {Format::RGBA8_UNORM, "rgba8_unorm"},
    {Format::RGBA8_SNORM, "rgba8_snorm"},
    {Format::RGBA8_UINT, "rgba8_uint"},
    {Format::RGBA8_SINT, "rgba8_sint"},
    {Format::RGBA8_SRGB, "rgba8_srgb"},
    {Format::BGRA8_UNORM, "bgra8_unorm"},
    {Format::BGRA8_SNORM, "bgra8_snorm"},
    {Format::BGRA8_UINT, "bgra8_uint"},
    {Format::BGRA8_SINT, "bgra8_sint"},
    {Format::BGRA8_SRGB, "bgra8_srgb"},
    {Format::R16_UNORM, "r16_unorm"},
    {Format::R16_SNORM, "r16_snorm"},
    {Format::R16_UINT, "r16_uint"},
    {Format::R16_SINT, "r16_sint"},
    {Format::R16_SFLOAT, "r16_sfloat"},
    {Format::RG16_UNORM, "rg16_unorm"},
    {Format::RG16_SNORM, "rg16_snorm"},
    {Format::RG16_UINT, "rg16_uint"},
    {Format::RG16_SINT, "rg16_sint"},
    {Format::RG16_SFLOAT, "rg16_sfloat"},
    {Format::RGB16_UNORM, "rgb16_unorm"},
    {Format::RGB16_SNORM, "rgb16_snorm"},
    {Format::RGB16_UINT, "rgb16_uint"},
    {Format::RGB16_SINT, "rgb16_sint"},
    {Format::RGB16_SFLOAT, "rgb16_sfloat"},
    {Format::RGBA16_UNORM, "rgba16_unorm"},
    {Format::RGBA16_SNORM, "rgba16_snorm"},
    {Format::RGBA16_UINT, "rgba16_uint"},
    {Format::RGBA16_SINT, "rgba16_sint"},
    {Format::RGBA16_SFLOAT, "rgba16_sfloat"},
    {Format::R32_UINT, "r32_uint"},
    {Format::R32_SINT, "r32_sint"},
    {Format::R32_SFLOAT, "r32_sfloat"},
    {Format::RG32_UINT, "rg32_uint"},
    {Format::RG32_SINT, "rg32_sint"},
    {Format::RG32_SFLOAT, "rg32_sfloat"},
    {Format::RGB32_UINT, "rgb32_uint"},
    {Format::RGB32_SINT, "rgb32_sint"},
    {Format::RGB32_SFLOAT, "rgb32_sfloat"},
    {Format::RGBA32_UINT, "rgba32_uint"},
    {Format::RGBA32_SINT, "rgba32_sint"},
    {Format::RGBA32_SFLOAT, "rgba32_sfloat"},
    {Format::R64_UINT, "r64_uint"},
    {Format::R64_SINT, "r64_sint"},
    {Format::R64_SFLOAT, "r64_sfloat"},
    {Format::RG64_UINT, "rg64_uint"},
    {Format::RG64_SINT, "rg64_sint"},
    {Format::RG64_SFLOAT, "rg64_sfloat"},
    {Format::RGB64_UINT, "rgb64_uint"},
    {Format::RGB64_SINT, "rgb64_sint"},
    {Format::RGB64_SFLOAT, "rgb64_sfloat"},
    {Format::RGBA64_UINT, "rgba64_uint"},
    {Format::RGBA64_SINT, "rgba64_sint"},
    {Format::RGBA64_SFLOAT, "rgba64_sfloat"},
    {Format::D16_UNORM, "d16_unorm"},
    {Format::X8_D24_UNORM_PACK32, "x8_d24_unorm_pack32"},
    {Format::D32_SFLOAT, "d32_sfloat"},
    {Format::S8_UINT, "s8_uint"},
    {Format::D16_UNORM_S8_UINT, "d16_unorm_s8_uint"},
    {Format::D24_UNORM_S8_UINT, "d24_unorm_s8_uint"},
    {Format::D32_SFLOAT_S8_UINT, "d32_sfloat_s8_uint"},
    {Format::BC1_RGB_UNORM, "bc1_rgb_unorm"},
    {Format::BC1_RGB_SRGB, "bc1_rgb_srgb"},
    {Format::BC1_RGBA_UNORM, "bc1_rgba_unorm"},
    {Format::BC1_RGBA_SRGB, "bc1_rgba_srgb"},
    {Format::BC2_UNORM, "bc2_unorm"},
    {Format::BC2_SRGB, "bc2_srgb"},
    {Format::BC3_UNORM, "bc3_unorm"},
    {Format::BC3_SRGB, "bc3_srgb"},
    {Format::BC4_UNORM, "bc4_unorm"},
    {Format::BC4_SNORM, "bc4_snorm"},
    {Format::BC5_UNORM, "bc5_unorm"},
    {Format::BC5_SNORM, "bc5_snorm"},
    {Format::BC6H_UFLOAT, "bc6h_ufloat"},
    {Format::BC6H_SFLOAT, "bc6h_sfloat"},
    {Format::BC7_UNORM, "bc7_unorm"},
    {Format::BC7_SRGB, "bc7_srgb"},
    {Format::ETC2_RGB8_UNORM, "etc2_rgb8_unorm"},
    {Format::ETC2_RGB8_SRGB, "etc2_rgb8_srgb"},
    {Format::ETC2_RGB8A1_UNORM, "etc2_rgb8a1_unorm"},
    {Format::ETC2_RGB8A1_SRGB, "etc2_rgb8a1_srgb"},
    {Format::ETC2_RGBA8_UNORM, "etc2_rgba8_unorm"},
    {Format::ETC2_RGBA8_SRGB, "etc2_rgba8_srgb"},
    {Format::ASTC_4x4_UNORM, "astc_4x4_unorm"},
    {Format::ASTC_4x4_SRGB, "astc_4x4_srgb"},
    {Format::ASTC_5x4_UNORM, "astc_5x4_unorm"},
    {Format::ASTC_5x4_SRGB, "astc_5x4_srgb"},
    {Format::ASTC_5x5_UNORM, "astc_5x5_unorm"},
    {Format::ASTC_5x5_SRGB, "astc_5x5_srgb"},
    {Format::ASTC_6x5_UNORM, "astc_6x5_unorm"},
};

uint32_t getFormatSize(Format format);

DescriptorSetLayoutPtr getOrCreateDescriptorSetLayout(const DescriptorSetLayoutInfo& info, DevicePtr device);
PipelineLayoutPtr getOrCreatePipelineLayout(const PipelineLayoutInfo& info, DevicePtr device);

bool hasDepth(Format format);
bool hasStencil(Format format);

ImagePtr defaultSampledImage(DevicePtr);
ImagePtr defaultStorageImage(DevicePtr);
ImageViewPtr defaultSampledImageView(DevicePtr);
ImageViewPtr defaultStorageImageView(DevicePtr);
BufferPtr defaultUniformBuffer(DevicePtr);
BufferPtr defaultStorageBuffer(DevicePtr);
BufferViewPtr defaultUniformBufferView(DevicePtr);
BufferViewPtr defaultStorageBufferView(DevicePtr);
SamplerInfo defaultLinearSampler(DevicePtr);

ImagePtr createImageFromBuffer(BufferPtr buffer,
                               uint32_t width,
                               uint32_t height,
                               Format format,
                               CommandBufferPtr cmdBuffer,
                               DevicePtr device);
ImageViewPtr createDefaultView(ImagePtr image, DevicePtr device);

bool isSparse(RHIImage* img);


} // namespace raum::rhi