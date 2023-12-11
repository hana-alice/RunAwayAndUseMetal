#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <map>
#include "VKUtils.h"
namespace raum::rhi {

const std::map<Format, FormatInfo> formatMap = {
    {Format::UNKNOWN, {VK_FORMAT_UNDEFINED, 0, 0}},
    {Format::A8_UNORM, {VK_FORMAT_A8_UNORM_KHR, 1, 1}},
    {Format::R8_UNORM, {VK_FORMAT_R8_UNORM, 1, 1}},
    {Format::R8_SNORM, {VK_FORMAT_R8_SNORM, 1, 1}},
    {Format::R8_UINT, {VK_FORMAT_R8_UINT, 1, 1}},
    {Format::R8_SINT, {VK_FORMAT_R8_SINT, 1, 1}},
    {Format::R8_SRGB, {VK_FORMAT_R8_SRGB, 1, 1}},
    {Format::R8G8_UNORM, {VK_FORMAT_R8G8_UNORM, 2, 1}},
    {Format::R8G8_SNORM, {VK_FORMAT_R8G8_SNORM, 2, 1}},
    {Format::R8G8_UINT, {VK_FORMAT_R8G8_UINT, 2, 1}},
    {Format::R8G8_SINT, {VK_FORMAT_R8G8_SINT, 2, 1}},
    {Format::R8G8_SRGB, {VK_FORMAT_R8G8_SRGB, 2, 1}},
    {Format::R8G8B8_UNORM, {VK_FORMAT_R8G8B8_UNORM, 3, 1}},
    {Format::R8G8B8_SNORM, {VK_FORMAT_R8G8B8_SNORM, 3, 1}},
    {Format::R8G8B8_UINT, {VK_FORMAT_R8G8B8_UINT, 3, 1}},
    {Format::R8G8B8_SINT, {VK_FORMAT_R8G8B8_SINT, 3, 1}},
    {Format::R8G8B8_SRGB, {VK_FORMAT_R8G8B8_SRGB, 3, 1}},
    {Format::B8G8R8_UNORM, {VK_FORMAT_B8G8R8_UNORM, 3, 1}},
    {Format::B8G8R8_SNORM, {VK_FORMAT_B8G8R8_SNORM, 3, 1}},
    {Format::B8G8R8_UINT, {VK_FORMAT_B8G8R8_UINT, 3, 1}},
    {Format::B8G8R8_SINT, {VK_FORMAT_B8G8R8_SINT, 3, 1}},
    {Format::B8G8R8_SRGB, {VK_FORMAT_B8G8R8_SRGB, 3, 1}},
    {Format::R8G8B8A8_UNORM, {VK_FORMAT_R8G8_UNORM, 4, 1}},
    {Format::R8G8B8A8_SNORM, {VK_FORMAT_R8G8_SNORM, 4, 1}},
    {Format::R8G8B8A8_UINT, {VK_FORMAT_R8G8_UINT, 4, 1}},
    {Format::R8G8B8A8_SINT, {VK_FORMAT_R8G8_SINT, 4, 1}},
    {Format::R8G8B8A8_SRGB, {VK_FORMAT_R8G8_SRGB, 4, 1}},
    {Format::B8G8R8A8_UNORM, {VK_FORMAT_B8G8R8_UNORM, 4, 1}},
    {Format::B8G8R8A8_SNORM, {VK_FORMAT_B8G8R8_SNORM, 4, 1}},
    {Format::B8G8R8A8_UINT, {VK_FORMAT_B8G8R8_UINT, 4, 1}},
    {Format::B8G8R8A8_SINT, {VK_FORMAT_B8G8R8_SINT, 4, 1}},
    {Format::B8G8R8A8_SRGB, {VK_FORMAT_B8G8R8_SRGB, 4, 1}},
    {Format::R16_UNORM, {VK_FORMAT_R16_UNORM, 2, 1}},
    {Format::R16_SNORM, {VK_FORMAT_R16_SNORM, 2, 1}},
    {Format::R16_UINT, {VK_FORMAT_R16_UINT, 2, 1}},
    {Format::R16_SINT, {VK_FORMAT_R16_SINT, 2, 1}},
    {Format::R16_SFLOAT, {VK_FORMAT_R16_SFLOAT, 2, 1}},
    {Format::R16G16_UNORM, {VK_FORMAT_R16G16_UNORM, 4, 1}},
    {Format::R16G16_SNORM, {VK_FORMAT_R16G16_SNORM, 4, 1}},
    {Format::R16G16_UINT, {VK_FORMAT_R16G16_UINT, 4, 1}},
    {Format::R16G16_SINT, {VK_FORMAT_R16G16_SINT, 4, 1}},
    {Format::R16G16_SFLOAT, {VK_FORMAT_R16G16_SFLOAT, 4, 1}},
    {Format::R16G16B16_UNORM, {VK_FORMAT_R16G16_UNORM, 6, 1}},
    {Format::R16G16B16_SNORM, {VK_FORMAT_R16G16_SNORM, 6, 1}},
    {Format::R16G16B16_UINT, {VK_FORMAT_R16G16_UNORM, 6, 1}},
    {Format::R16G16B16_SINT, {VK_FORMAT_R16G16_SFLOAT, 6, 1}},
    {Format::R16G16B16_SFLOAT, {VK_FORMAT_R16G16_SFLOAT, 6, 1}},
    {Format::R16G16B16A16_UNORM, {VK_FORMAT_R16G16B16A16_UNORM, 8, 1}},
    {Format::R16G16B16A16_SNORM, {VK_FORMAT_R16G16B16A16_SNORM, 8, 1}},
    {Format::R16G16B16A16_UINT, {VK_FORMAT_R16G16B16A16_UINT, 8, 1}},
    {Format::R16G16B16A16_SINT, {VK_FORMAT_R16G16B16A16_SINT, 8, 1}},
    {Format::R16G16B16A16_SFLOAT, {VK_FORMAT_R16G16B16A16_SFLOAT, 8, 1}},
    {Format::R32_UINT, {VK_FORMAT_R32_UINT, 4, 1}},
    {Format::R32_SINT, {VK_FORMAT_R32_SINT, 4, 1}},
    {Format::R32_SFLOAT, {VK_FORMAT_R32_SFLOAT, 4, 1}},
    {Format::R32G32_UINT, {VK_FORMAT_R32G32_UINT, 8, 1}},
    {Format::R32G32_SINT, {VK_FORMAT_R32G32_SINT, 8, 1}},
    {Format::R32G32_SFLOAT, {VK_FORMAT_R32G32_SFLOAT, 8, 1}},
    {Format::R32G32B32_UINT, {VK_FORMAT_R32G32B32_UINT, 12, 1}},
    {Format::R32G32B32_SINT, {VK_FORMAT_R32G32B32_SINT, 12, 1}},
    {Format::R32G32B32_SFLOAT, {VK_FORMAT_R32G32B32_SFLOAT, 12, 1}},
    {Format::R32G32B32A32_UINT, {VK_FORMAT_R32G32B32A32_UINT, 16, 1}},
    {Format::R32G32B32A32_SINT, {VK_FORMAT_R32G32B32A32_SINT, 16, 1}},
    {Format::R32G32B32A32_SFLOAT, {VK_FORMAT_R32G32B32A32_SFLOAT, 16, 1}},
    {Format::R64_UINT, {VK_FORMAT_R64_UINT, 8, 1}},
    {Format::R64_SINT, {VK_FORMAT_R64_SINT, 8, 1}},
    {Format::R64_SFLOAT, {VK_FORMAT_R64_SFLOAT, 8, 1}},
    {Format::R64G64_UINT, {VK_FORMAT_R64G64_UINT, 16, 1}},
    {Format::R64G64_SINT, {VK_FORMAT_R64G64_SINT, 16, 1}},
    {Format::R64G64_SFLOAT, {VK_FORMAT_R64G64_SFLOAT, 16, 1}},
    {Format::R64G64B64_UINT, {VK_FORMAT_R64G64B64_UINT, 24, 1}},
    {Format::R64G64B64_SINT, {VK_FORMAT_R64G64B64_SINT, 24, 1}},
    {Format::R64G64B64_SFLOAT, {VK_FORMAT_R64G64B64_SFLOAT, 24, 1}},
    {Format::R64G64B64A64_UINT, {VK_FORMAT_R64G64B64A64_UINT, 32, 1}},
    {Format::R64G64B64A64_SINT, {VK_FORMAT_R64G64B64A64_SINT, 32, 1}},
    {Format::R64G64B64A64_SFLOAT, {VK_FORMAT_R64G64B64A64_SFLOAT, 32, 1}},
    {Format::D16_UNORM, {VK_FORMAT_D16_UNORM, 2, 1}},
    {Format::X8_D24_UNORM_PACK32, {VK_FORMAT_D24_UNORM_S8_UINT, 4, 1}},
    {Format::D32_SFLOAT, {VK_FORMAT_D32_SFLOAT, 4, 1}},
    {Format::S8_UINT, {VK_FORMAT_S8_UINT, 1, 1}},
    {Format::D24_UNORM_S8_UINT, {VK_FORMAT_D24_UNORM_S8_UINT, 4, 1}},
    {Format::D32_SFLOAT_S8_UINT, {VK_FORMAT_D32_SFLOAT_S8_UINT, 8, 1}},
    {Format::BC1_RGB_UNORM_BLOCK, {VK_FORMAT_BC1_RGB_UNORM_BLOCK, 8, 16}},
    {Format::BC1_RGB_SRGB_BLOCK, {VK_FORMAT_BC1_RGB_SRGB_BLOCK, 8, 16}},
    {Format::BC1_RGBA_UNORM_BLOCK, {VK_FORMAT_BC1_RGBA_UNORM_BLOCK, 8, 16}},
    {Format::BC1_RGBA_SRGB_BLOCK, {VK_FORMAT_BC1_RGBA_SRGB_BLOCK, 8, 16}},
    {Format::BC2_UNORM_BLOCK, {VK_FORMAT_BC2_UNORM_BLOCK, 16, 16}},
    {Format::BC2_SRGB_BLOCK, {VK_FORMAT_BC2_SRGB_BLOCK, 16, 16}},
    {Format::BC3_UNORM_BLOCK, {VK_FORMAT_BC3_UNORM_BLOCK, 16, 16}},
    {Format::BC3_SRGB_BLOCK, {VK_FORMAT_BC3_SRGB_BLOCK, 16, 16}},
    {Format::BC4_UNORM_BLOCK, {VK_FORMAT_BC4_UNORM_BLOCK, 8, 16}},
    {Format::BC4_SNORM_BLOCK, {VK_FORMAT_BC4_SNORM_BLOCK, 8, 16}},
    {Format::BC5_UNORM_BLOCK, {VK_FORMAT_BC5_UNORM_BLOCK, 16, 16}},
    {Format::BC5_SNORM_BLOCK, {VK_FORMAT_BC5_SNORM_BLOCK, 16, 16}},
    {Format::BC6H_UFLOAT_BLOCK, {VK_FORMAT_BC6H_UFLOAT_BLOCK, 16, 16}},
    {Format::BC6H_SFLOAT_BLOCK, {VK_FORMAT_BC6H_SFLOAT_BLOCK, 16, 16}},
    {Format::BC7_UNORM_BLOCK, {VK_FORMAT_BC7_UNORM_BLOCK, 16, 16}},
    {Format::BC7_SRGB_BLOCK, {VK_FORMAT_BC7_SRGB_BLOCK, 16, 16}},
    {Format::ETC2_R8G8B8_UNORM_BLOCK, {VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK, 8, 16}},
    {Format::ETC2_R8G8B8_SRGB_BLOCK, {VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK, 8, 16}},
    {Format::ETC2_R8G8B8A1_UNORM_BLOCK, {VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK, 8, 16}},
    {Format::ETC2_R8G8B8A1_SRGB_BLOCK, {VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK, 8, 16}},
    {Format::ETC2_R8G8B8A8_UNORM_BLOCK, {VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK, 16, 16}},
    {Format::ETC2_R8G8B8A8_SRGB_BLOCK, {VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK, 16, 16}},
    {Format::ASTC_4x4_UNORM_BLOCK, {VK_FORMAT_ASTC_4x4_UNORM_BLOCK, 16, 16}},
    {Format::ASTC_4x4_SRGB_BLOCK, {VK_FORMAT_ASTC_4x4_SRGB_BLOCK, 16, 16}},
    {Format::ASTC_5x4_UNORM_BLOCK, {VK_FORMAT_ASTC_5x4_UNORM_BLOCK, 16, 20}},
    {Format::ASTC_5x4_SRGB_BLOCK, {VK_FORMAT_ASTC_5x4_SRGB_BLOCK, 16, 20}},
    {Format::ASTC_5x5_UNORM_BLOCK, {VK_FORMAT_ASTC_5x5_UNORM_BLOCK, 16, 25}},
    {Format::ASTC_5x5_SRGB_BLOCK, {VK_FORMAT_ASTC_5x5_SRGB_BLOCK, 16, 25}},
    {Format::ASTC_6x5_UNORM_BLOCK, {VK_FORMAT_ASTC_6x5_UNORM_BLOCK, 16, 30}},
};

FormatInfo formatInfo(Format format) {
    return formatMap.at(format);
}

VkVertexInputRate mapRate(InputRate rate) {
    switch (rate) {
        case InputRate::PER_VERTEX:
            return VK_VERTEX_INPUT_RATE_VERTEX;
        case InputRate::PER_INSTANCE:
            return VK_VERTEX_INPUT_RATE_INSTANCE;
    }
    return VK_VERTEX_INPUT_RATE_VERTEX;
}

VkCullModeFlags cullMode(CullMode mode) {
    VkCullModeFlags res = VK_CULL_MODE_NONE;
    switch (mode) {
        case CullMode::NONE:
            res = VK_CULL_MODE_NONE;
            break;
        case CullMode::FRONT:
            res = VK_CULL_MODE_FRONT_BIT;
            break;
        case CullMode::BACK:
            res = VK_CULL_MODE_BACK_BIT;
            break;
        case CullMode::FRONT_AND_BACK:
            res = VK_CULL_MODE_FRONT_AND_BACK;
            break;
        default:
            break;  
    }
    return res;
}

VkPolygonMode polygonMode(PolygonMode mode) {
    VkPolygonMode res = VK_POLYGON_MODE_FILL;
    switch (mode) {
        case PolygonMode::POINT:
            res = VK_POLYGON_MODE_POINT;
            break;
        case PolygonMode::LINE:
            res = VK_POLYGON_MODE_LINE;
            break;
        case PolygonMode::FILL:
            res = VK_POLYGON_MODE_FILL;
            break;
        default:
            break;
    }
    return res;
}

VkFrontFace frontFace(FrontFace face) {
    VkFrontFace res = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    switch (face) {
        case FrontFace::COUNTER_CLOCKWISE:
            res = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            break;
        case FrontFace::CLOCKWISE:
            res = VK_FRONT_FACE_CLOCKWISE;
            break;
        default:
            break;
    }
    return res;
}
VkSampleCountFlagBits sampleCount(uint32_t samples) {
    VkSampleCountFlagBits res = VK_SAMPLE_COUNT_1_BIT;
    switch (samples) {
        case 1:
            res = VK_SAMPLE_COUNT_1_BIT;
            break;
        case 2:
            res = VK_SAMPLE_COUNT_2_BIT;
            break;
        case 4:
            res = VK_SAMPLE_COUNT_4_BIT;
            break;
        case 8:
            res = VK_SAMPLE_COUNT_8_BIT;
            break;
        case 16:
            res = VK_SAMPLE_COUNT_16_BIT;
            break;
    }
    return res;
}

VkCompareOp compareOp(CompareOp op) {
    VkCompareOp res = VK_COMPARE_OP_ALWAYS;
    switch (op) {
        case CompareOp::NEVER:
            res = VK_COMPARE_OP_NEVER;
            break;
        case CompareOp::LESS:
            res = VK_COMPARE_OP_LESS;
            break;
        case CompareOp::EQUAL:
            res = VK_COMPARE_OP_EQUAL;
            break;
        case CompareOp::LESS_OR_EQUAL:
            res = VK_COMPARE_OP_LESS_OR_EQUAL;
            break;
        case CompareOp::GREATER:
            res = VK_COMPARE_OP_GREATER;
            break;
        case CompareOp::NOT_EQUAL:
            res = VK_COMPARE_OP_NOT_EQUAL;
            break;
        case CompareOp::GREATER_OR_EQUAL:
            res = VK_COMPARE_OP_GREATER_OR_EQUAL;
            break;
        case CompareOp::ALWAYS:
            res = VK_COMPARE_OP_ALWAYS;
            break;
    }
    return res;
}

VkStencilOp stencilOp(StencilOp op) {
    VkStencilOp res = VK_STENCIL_OP_KEEP;
    switch (op) {
        case StencilOp::KEEP:
            res = VK_STENCIL_OP_KEEP;
            break;
        case StencilOp::ZERO:
            res = VK_STENCIL_OP_ZERO;
            break;
        case StencilOp::REPLACE:
            res = VK_STENCIL_OP_REPLACE;
            break;
        case StencilOp::INCREMENT_AND_CLAMP:
            res = VK_STENCIL_OP_INCREMENT_AND_CLAMP;
            break;
        case StencilOp::DECREMENT_AND_CLAMP:
            res = VK_STENCIL_OP_DECREMENT_AND_CLAMP;
            break;
        case StencilOp::INVERT:
            res = VK_STENCIL_OP_INVERT;
            break;
        case StencilOp::INCREMENT_AND_WRAP:
            res = VK_STENCIL_OP_INCREMENT_AND_WRAP;
            break;
        case StencilOp::DECREMENT_AND_WRAP:
            res = VK_STENCIL_OP_DECREMENT_AND_WRAP;
            break;
    }
    return res;
}

VkBlendFactor blendFactor(BlendFactor factor) {
    VkBlendFactor res = VK_BLEND_FACTOR_ONE;
    switch (factor) {
        case BlendFactor::ZERO:
            res = VK_BLEND_FACTOR_ZERO;
            break;
        case BlendFactor::ONE:
            res = VK_BLEND_FACTOR_ONE;
            break;
        case BlendFactor::SRC_COLOR:
            res = VK_BLEND_FACTOR_SRC_COLOR;
            break;
        case BlendFactor::ONE_MINUS_SRC_COLOR:
            res = VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
            break;
        case BlendFactor::DST_COLOR:
            res = VK_BLEND_FACTOR_DST_COLOR;
            break;
        case BlendFactor::ONE_MINUS_DST_COLOR:
            res = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
            break;
        case BlendFactor::SRC_ALPHA:
            res = VK_BLEND_FACTOR_SRC_ALPHA;
            break;
        case BlendFactor::ONE_MINUS_SRC_ALPHA:
            res = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
        case BlendFactor::DST_ALPHA:
            res = VK_BLEND_FACTOR_DST_ALPHA;
            break;
        case BlendFactor::ONE_MINUS_DST_ALPHA:
            res = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            break;
        case BlendFactor::CONSTANT_COLOR:
            res = VK_BLEND_FACTOR_CONSTANT_COLOR;
            break;
        case BlendFactor::ONE_MINUS_CONSTANT_COLOR:
            res = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
            break;
        case BlendFactor::CONSTANT_ALPHA:
            res = VK_BLEND_FACTOR_CONSTANT_ALPHA;
            break;
        case BlendFactor::ONE_MINUS_CONSTANT_ALPHA:
            res = VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
            break;
        case BlendFactor::SRC_ALPHA_SATURATE:
            res = VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
            break;
    }
    return res;
}

VkBlendOp blendOp(BlendOp op) {
    VkBlendOp res = VK_BLEND_OP_ADD;
    switch (op) {
        case BlendOp::ADD:
            res = VK_BLEND_OP_ADD;
            break;
        case BlendOp::SUBTRACT:
            res = VK_BLEND_OP_SUBTRACT;
            break;
        case BlendOp::REVERSE_SUBTRACT:
            res = VK_BLEND_OP_REVERSE_SUBTRACT;
            break;
        case BlendOp::MIN:
            res = VK_BLEND_OP_MIN;
            break;
        case BlendOp::MAX:
            res = VK_BLEND_OP_MAX;
            break;
    }
    return res;
}

VkColorComponentFlags colorComponentFlags(Channel channel) {
    return static_cast<VkColorComponentFlags>(channel);
}

VkLogicOp logicOp(LogicOp op) {
    VkLogicOp res = VK_LOGIC_OP_CLEAR;
    switch (op) {
        case LogicOp::CLEAR:
            res = VK_LOGIC_OP_CLEAR;
            break;
        case LogicOp::AND:
            res = VK_LOGIC_OP_AND;
            break;
        case LogicOp::AND_REVERSE:
            res = VK_LOGIC_OP_AND_REVERSE;
            break;
        case LogicOp::COPY:
            res = VK_LOGIC_OP_COPY;
            break;
        case LogicOp::AND_INVERTED:
            res = VK_LOGIC_OP_AND_INVERTED;
            break;
        case LogicOp::NOOP:
            res = VK_LOGIC_OP_NO_OP;
            break;
        case LogicOp::XOR:
            res = VK_LOGIC_OP_XOR;
            break;
        case LogicOp::OR:
            res = VK_LOGIC_OP_OR;
            break;
        case LogicOp::NOR:
            res = VK_LOGIC_OP_NOR;
            break;
        case LogicOp::EQUIVALENT:
            res = VK_LOGIC_OP_EQUIVALENT;
            break;
        case LogicOp::INVERT:
            res = VK_LOGIC_OP_INVERT;
            break;
        case LogicOp::OR_REVERSE:
            res = VK_LOGIC_OP_OR_REVERSE;
            break;
        case LogicOp::COPY_INVERTED:
            res = VK_LOGIC_OP_COPY_INVERTED;
            break;
        case LogicOp::OR_INVERTED:
            res = VK_LOGIC_OP_OR_INVERTED;
            break;
        case LogicOp::NAND:
            res = VK_LOGIC_OP_NAND;
            break;
        case LogicOp::SET:
            res = VK_LOGIC_OP_SET;
            break;
    }
    return res;
}

} // namespace raum::rhi
