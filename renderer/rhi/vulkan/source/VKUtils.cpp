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

VkCullModeFlags cullMode(FaceMode mode) {
    VkCullModeFlags res = VK_CULL_MODE_NONE;
    switch (mode) {
        case FaceMode::NONE:
            res = VK_CULL_MODE_NONE;
            break;
        case FaceMode::FRONT:
            res = VK_CULL_MODE_FRONT_BIT;
            break;
        case FaceMode::BACK:
            res = VK_CULL_MODE_BACK_BIT;
            break;
        case FaceMode::FRONT_AND_BACK:
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

VkDescriptorType descriptorType(DescriptorType type) {
    VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    switch (type) {
        case DescriptorType::SAMPLER:
            descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case DescriptorType::SAMPLED_IMAGE:
            descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        case DescriptorType::STORAGE_IMAGE:
            descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        case DescriptorType::UNIFORM_TEXEL_BUFFER:
            descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            break;
        case DescriptorType::STORAGE_TEXEL_BUFFER:
            descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            break;
        case DescriptorType::UNIFORM_BUFFER:
            descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        case DescriptorType::STORAGE_BUFFER:
            descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        case DescriptorType::UNIFORM_BUFFER_DYNAMIC:
            descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            break;
        case DescriptorType::STORAGE_BUFFER_DYNAMIC:
            descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            break;
        case DescriptorType::INPUT_ATTACHMENT:
            descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            break;
    }
    return descriptorType;
}

VkShaderStageFlags shaderStageFlags(ShaderStage stage) {
    VkShaderStageFlags res;
    if (test(stage, ShaderStage::VERTEX)) {
        res = VK_SHADER_STAGE_VERTEX_BIT;
    }
    if (test(stage, ShaderStage::FRAGMENT)) {
        res = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    if (test(stage, ShaderStage::TASK)) {
        res = VK_SHADER_STAGE_TASK_BIT_EXT;
    }
    if (test(stage, ShaderStage::MESH)) {
        res = VK_SHADER_STAGE_MESH_BIT_EXT;
    }
    if (test(stage, ShaderStage::COMPUTE)) {
        res = VK_SHADER_STAGE_COMPUTE_BIT;
    }
    return res;
}

VkAttachmentLoadOp loadOp(LoadOp op) {
    VkAttachmentLoadOp res = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    switch (op) {
        case LoadOp::LOAD:
            res = VK_ATTACHMENT_LOAD_OP_LOAD;
            break;
        case LoadOp::CLEAR:
            res = VK_ATTACHMENT_LOAD_OP_CLEAR;
            break;
        case LoadOp::DISCARD:
            res = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            break;
    }
    return res;
}

VkAttachmentStoreOp storeOp(StoreOp op) {
    VkAttachmentStoreOp res = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    switch (op) {
        case StoreOp::STORE:
            res = VK_ATTACHMENT_STORE_OP_STORE;
            break;
        case StoreOp::CLEAR:
            res = VK_ATTACHMENT_STORE_OP_STORE;
            break;
    }
    return res;
}

VkImageLayout imageLayout(ImageLayout layout) {
    VkImageLayout res = VK_IMAGE_LAYOUT_UNDEFINED;
    switch (layout) {
        case ImageLayout::UNDEFINED:
            res = VK_IMAGE_LAYOUT_UNDEFINED;
            break;
        case ImageLayout::GENERAL:
            res = VK_IMAGE_LAYOUT_GENERAL;
            break;
        case ImageLayout::COLOR_ATTACHMENT_OPTIMAL:
            res = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        case ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            res = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            break;
        case ImageLayout::DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            res = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            break;
        case ImageLayout::SHADER_READ_ONLY_OPTIMAL:
            res = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            break;
        case ImageLayout::TRANSFER_SRC_OPTIMAL:
            res = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            break;
        case ImageLayout::TRANSFER_DST_OPTIMAL:
            res = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            break;
        case ImageLayout::PREINITIALIZED:
            res = VK_IMAGE_LAYOUT_PREINITIALIZED;
            break;
        case ImageLayout::DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            res = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
            break;
        case ImageLayout::DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            res = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            break;
        case ImageLayout::DEPTH_ATTACHMENT_OPTIMAL:
            res = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
            break;
        case ImageLayout::DEPTH_READ_ONLY_OPTIMAL:
            res = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
            break;
        case ImageLayout::STENCIL_ATTACHMENT_OPTIMAL:
            res = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
            break;
        case ImageLayout::STENCIL_READ_ONLY_OPTIMAL:
            res = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
            break;
        case ImageLayout::READ_ONLY_OPTIMAL:
            res = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL;
            break;
        case ImageLayout::ATTACHMENT_OPTIMAL:
            res = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            break;
        case ImageLayout::PRESENT:
            res = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            break;
        case ImageLayout::SHADING_RATE:
            res = VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;
            break;
    }
    return res;
}

VkPipelineStageFlags pipelineStageFlags(PipelineStage stage) {
    VkPipelineStageFlags res;
    switch (stage) {
        case PipelineStage::TOP_OF_PIPE:
            res = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case PipelineStage::DRAW_INDIRECT:
            res = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            break;
        case PipelineStage::VERTEX_INPUT:
            res = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            break;
        case PipelineStage::VERTEX_SHADER:
            res = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            break;
        case PipelineStage::FRAGMENT_SHADER:
            res = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            break;
        case PipelineStage::EARLY_FRAGMENT_TESTS:
            res = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        case PipelineStage::LATE_FRAGMENT_TESTS:
            res = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case PipelineStage::COLOR_ATTACHMENT_OUTPUT:
            res = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case PipelineStage::COMPUTE_SHADER:
            res = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
            break;
        case PipelineStage::TRANSFER:
            res = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case PipelineStage::BOTTOM_OF_PIPE:
            res = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            break;
        case PipelineStage::HOST:
            res = VK_PIPELINE_STAGE_HOST_BIT;
            break;
        case PipelineStage::TASK_SHADER:
            res = VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;
            break;
        case PipelineStage::MESH_SHADER:
            res = VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
            break;
    }
    return res;
}

VkAccessFlags accessFlags(AccessFlags flags) {
    VkAccessFlags res = VK_ACCESS_NONE;

    if (test(flags, AccessFlags::INDIRECT_COMMAND_READ)) {
        res |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    }
    if (test(flags, AccessFlags::INDEX_READ)) {
        res |= VK_ACCESS_INDEX_READ_BIT;
    }
    if (test(flags, AccessFlags::VERTEX_ATTRIBUTE_READ)) {
        res |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    }
    if (test(flags, AccessFlags::UNIFORM_READ)) {
        res |= VK_ACCESS_UNIFORM_READ_BIT;
    }
    if (test(flags, AccessFlags::INPUT_ATTACHMENT_READ)) {
        res |= VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    }
    if (test(flags, AccessFlags::SHADER_READ)) {
        res |= VK_ACCESS_SHADER_READ_BIT;
    }
    if (test(flags, AccessFlags::SHADER_WRITE)) {
        res |= VK_ACCESS_SHADER_WRITE_BIT;
    }
    if (test(flags, AccessFlags::COLOR_ATTACHMENT_READ)) {
        res |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
    }
    if (test(flags, AccessFlags::COLOR_ATTACHMENT_WRITE)) {
        res |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    }
    if (test(flags, AccessFlags::DEPTH_STENCIL_ATTACHMENT_READ)) {
        res |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    }
    if (test(flags, AccessFlags::DEPTH_STENCIL_ATTACHMENT_WRITE)) {
        res |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    if (test(flags, AccessFlags::TRANSFER_READ)) {
        res |= VK_ACCESS_TRANSFER_READ_BIT;
    }
    if (test(flags, AccessFlags::TRANSFER_WRITE)) {
        res |= VK_ACCESS_TRANSFER_WRITE_BIT;
    }
    if (test(flags, AccessFlags::HOST_READ)) {
        res |= VK_ACCESS_HOST_READ_BIT;
    }
    if (test(flags, AccessFlags::HOST_WRITE)) {
        res |= VK_ACCESS_HOST_WRITE_BIT;
    }
    if (test(flags, AccessFlags::MEMORY_READ)) {
        res |= VK_ACCESS_MEMORY_READ_BIT;
    }
    if (test(flags, AccessFlags::MEMORY_WRITE)) {
        res |= VK_ACCESS_MEMORY_WRITE_BIT;
    }
    return res;
}

VkDependencyFlags dependencyFlags(DependencyFlags flags) {
    VkDependencyFlags res;
    if (test(flags, DependencyFlags::BY_REGION)) {
        res |= VK_DEPENDENCY_BY_REGION_BIT;
    }
    if (test(flags, DependencyFlags::VIEW_LOCAL)) {
        res |= VK_DEPENDENCY_VIEW_LOCAL_BIT;
    }
    if (test(flags, DependencyFlags::DEVICE_GROUP)) {
        res |= VK_DEPENDENCY_DEVICE_GROUP_BIT;
    }
    if (test(flags, DependencyFlags::FEEDBACK_LOOP)) {
        res |= VK_DEPENDENCY_FEEDBACK_LOOP_BIT_EXT;
    }
    return res;
}

VkImageType imageType(ImageType type) {
    VkImageType res = VK_IMAGE_TYPE_2D;
    switch (type) {
        case ImageType::IMAGE_1D:
            res = VK_IMAGE_TYPE_1D;
            break;
        case ImageType::IMAGE_2D:
            res = VK_IMAGE_TYPE_2D;
            break;
        case ImageType::IMAGE_3D:
            res = VK_IMAGE_TYPE_3D;
            break;
    }
    return res;
}

VkImageCreateFlags imageFlag(ImageFlag flag) {
    VkImageCreateFlags res;
    if (test(flag, ImageFlag::SPARSE_BINDING)) {
        res |= VK_IMAGE_CREATE_SPARSE_BINDING_BIT;
    }
    if (test(flag, ImageFlag::SPARSE_RESIDENCY)) {
        res |= VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT;
    }
    if (test(flag, ImageFlag::SPARSE_ALIASED)) {
        res |= VK_IMAGE_CREATE_SPARSE_ALIASED_BIT;
    }
    if (test(flag, ImageFlag::MUTABLE_FORMAT)) {
        res |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    }
    if (test(flag, ImageFlag::ALIAS)) {
        res |= VK_IMAGE_CREATE_ALIAS_BIT;
    }
    return res;
}

VkImageUsageFlags imageUsage(ImageUsage usage) {
    VkImageUsageFlags res;
    if (test(usage, ImageUsage::SAMPLED)) {
        res |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (test(usage, ImageUsage::STORAGE)) {
        res |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (test(usage, ImageUsage::TRANSFER_SRC)) {
        res |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (test(usage, ImageUsage::TRANSFER_DST)) {
        res |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (test(usage, ImageUsage::COLOR_ATTACHMENT)) {
        res |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (test(usage, ImageUsage::DEPTH_STENCIL_ATTACHMENT)) {
        res |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    if (test(usage, ImageUsage::TRANSIENT)) {
        res |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    }
    if (test(usage, ImageUsage::INPUT_ATTACHMENT)) {
        res |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }
    if (test(usage, ImageUsage::SHADING_RATE)) {
        res |= VK_IMAGE_USAGE_SHADING_RATE_IMAGE_BIT_NV;
    }
    if (test(usage, ImageUsage::ATTACHMENT_FEEDBACK_LOOP)) {
        res |= VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT;
    }
    return res;
}

VkSharingMode sharingMode(SharingMode mode) {
    VkSharingMode sharingMode;
    switch (mode) {
        case SharingMode::CONCURRENT:
            sharingMode = VK_SHARING_MODE_CONCURRENT;
            break;
        case SharingMode::EXCLUSIVE:
            sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            break;
    }
    return sharingMode;
}

VkBufferCreateFlags bufferFlag(BufferFlag flag) {
    VkBufferCreateFlags res;
    if (test(flag, BufferFlag::SPARSE_BINDING)) {
        res |= VK_BUFFER_CREATE_SPARSE_BINDING_BIT;
    }
    if (test(flag, BufferFlag::SPARSE_RESIDENCY)) {
        res |= VK_BUFFER_CREATE_SPARSE_RESIDENCY_BIT;
    }
    if (test(flag, BufferFlag::SPARSE_ALIASED)) {
        res |= VK_BUFFER_CREATE_SPARSE_ALIASED_BIT;
    }
    return res;
}

VkImageViewType viewType(ImageViewType viewType) {
    VkImageViewType res = VK_IMAGE_VIEW_TYPE_2D;
    switch (viewType) {
        case ImageViewType::IMAGE_VIEW_1D:
            res = VK_IMAGE_VIEW_TYPE_1D;
            break;
        case ImageViewType::IMAGE_VIEW_2D:
            res = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case ImageViewType::IMAGE_VIEW_3D:
            res = VK_IMAGE_VIEW_TYPE_3D;
            break;
        case ImageViewType::IMAGE_VIEW_CUBE:
            res = VK_IMAGE_VIEW_TYPE_CUBE;
            break;
        case ImageViewType::IMAGE_VIEW_1D_ARRAY:
            res = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;
        case ImageViewType::IMAGE_VIEW_2D_ARRAY:
            res = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        case ImageViewType::IMAGE_VIEW_CUBE_ARRAY:
            res = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            break;
    }
    return res;
}

VkComponentSwizzle componentSwizzle(ComponentSwizzle component) {
    VkComponentSwizzle res = VK_COMPONENT_SWIZZLE_IDENTITY;
    switch (component) {
        case ComponentSwizzle::IDENTITY:
            res = VK_COMPONENT_SWIZZLE_IDENTITY;
            break;
        case ComponentSwizzle::ZERO:
            res = VK_COMPONENT_SWIZZLE_ZERO;
            break;
        case ComponentSwizzle::ONE:
            res = VK_COMPONENT_SWIZZLE_ONE;
            break;
        case ComponentSwizzle::R:
            res = VK_COMPONENT_SWIZZLE_R;
            break;
        case ComponentSwizzle::G:
            res = VK_COMPONENT_SWIZZLE_G;
            break;
        case ComponentSwizzle::B:
            res = VK_COMPONENT_SWIZZLE_B;
            break;
        case ComponentSwizzle::A:
            res = VK_COMPONENT_SWIZZLE_A;
            break;
    }
    return res;
}

VkImageAspectFlags aspectMask(AspectMask mask) {
    VkImageAspectFlags res = VK_IMAGE_ASPECT_NONE;
    if (test(mask, AspectMask::COLOR)) {
        res |= VK_IMAGE_ASPECT_COLOR_BIT;
    }
    if (test(mask, AspectMask::DEPTH)) {
        res |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    if (test(mask, AspectMask::STENCIL)) {
        res |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    if (test(mask, AspectMask::METADATA)) {
        res |= VK_IMAGE_ASPECT_METADATA_BIT;
    }
    if (test(mask, AspectMask::PLANE_0)) {
        res |= VK_IMAGE_ASPECT_PLANE_0_BIT;
    }
    if (test(mask, AspectMask::PLANE_1)) {
        res |= VK_IMAGE_ASPECT_PLANE_1_BIT;
    }
    if (test(mask, AspectMask::PLANE_2)) {
        res |= VK_IMAGE_ASPECT_PLANE_2_BIT;
    }
    return res;
}

} // namespace raum::rhi
