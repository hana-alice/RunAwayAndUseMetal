#pragma once
#include <stdint.h>
#include <array>
#include <string>
#include <vector>
#include "define.h"
#include "glm.hpp"
namespace raum::rhi {

#define OPERABLE(T)                                                                                                               \
    inline T operator|(T lhs, T rhs) {                                                                                            \
        return static_cast<T>(static_cast<std::underlying_type<T>::type>(lhs) | static_cast<std::underlying_type<T>::type>(rhs)); \
    }                                                                                                                             \
    inline T operator&(T lhs, T rhs) {                                                                                            \
        return static_cast<T>(static_cast<std::underlying_type<T>::type>(lhs) & static_cast<std::underlying_type<T>::type>(rhs)); \
    }                                                                                                                             \
    inline bool test(T lhs, T rhs) {                                                                                              \
        return static_cast<std::underlying_type<T>::type>(lhs & rhs);                                                             \
    }

class RHIShader;
class RHIPipelineLayout;
class RHIRenderPass;
class RHIDescriptorSetLayout;
class RHIDescriptorSet;
class RHIImage;
class RHIImageView;
class RHIBuffer;
class RHIBufferView;
class RHIQueue;
class RHIFrameBuffer;

static constexpr uint32_t FRAMES_IN_FLIGHT{3};

using Vector3I = glm::ivec3;
using Vector4I = glm::ivec4;
using Vector3U = glm::uvec3;
using Vector4U = glm::uvec4;
using Vector3F = glm::vec3;
using Vector4F = glm::vec4;

enum class API : unsigned char {
    VULKAN,
};

enum class Format : uint32_t {
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

enum class FormatType {
    COLOR_UINT,
    COLOR_INT,
    COLOR_FLOAT,
    COLOR_UNFILTER_FLOAT,
    DEPTH,
    STENCIL,
    DEPTH_STENCIL,
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

enum class ShaderStage : uint32_t {
    VERTEX = 1,
    TASK = 1 << 1,
    MESH = 1 << 2,
    FRAGMENT = 1 << 3,
    COMPUTE = 1 << 4,
};
OPERABLE(ShaderStage)

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

struct SamplerInfo {
};

enum class ImageType : uint8_t {
    IMAGE_1D,
    IMAGE_2D,
    IMAGE_3D,
};

enum class ImageUsage : uint32_t {
    SAMPLED = 1 << 0,
    STORAGE = 1 << 1,
    TRANSFER_SRC = 1 << 2,
    TRANSFER_DST = 1 << 3,
    COLOR_ATTACHMENT = 1 << 4,
    DEPTH_STENCIL_ATTACHMENT = 1 << 5,
    INPUT_ATTACHMENT = 1 << 6,
    TRANSIENT = 1 << 7,
    SHADING_RATE = 1 << 8,
    ATTACHMENT_FEEDBACK_LOOP = 1 << 9,
};
OPERABLE(ImageUsage)

enum class SharingMode : uint8_t {
    CONCURRENT,
    EXCLUSIVE,
};

enum class ImageFlag : uint8_t {
    NONE = 0,
    SPARSE_BINDING = 1 << 0,
    SPARSE_RESIDENCY = 1 << 1,
    SPARSE_ALIASED = 1 << 2,
    MUTABLE_FORMAT = 1 << 3,
    ALIAS = 1 << 4,
};
OPERABLE(ImageFlag)

enum class ImageLayout : uint8_t {
    UNDEFINED,
    GENERAL,
    COLOR_ATTACHMENT_OPTIMAL,
    DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    DEPTH_STENCIL_READ_ONLY_OPTIMAL,
    SHADER_READ_ONLY_OPTIMAL,
    TRANSFER_SRC_OPTIMAL,
    TRANSFER_DST_OPTIMAL,
    PREINITIALIZED,
    DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
    DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
    DEPTH_ATTACHMENT_OPTIMAL,
    DEPTH_READ_ONLY_OPTIMAL,
    STENCIL_ATTACHMENT_OPTIMAL,
    STENCIL_READ_ONLY_OPTIMAL,
    READ_ONLY_OPTIMAL,
    ATTACHMENT_OPTIMAL,
    PRESENT,
    SHADING_RATE,
};

struct ImageInfo {
    ImageType type{ImageType::IMAGE_2D};
    ImageUsage usage{ImageUsage::SAMPLED};
    SharingMode shareingMode{SharingMode::EXCLUSIVE};
    ImageFlag imageFlag{ImageFlag::NONE};
    ImageLayout intialLayout{ImageLayout::UNDEFINED};
    Format format{Format::UNKNOWN};
    uint32_t sliceCount{0};
    uint32_t mipCount{0};
    uint32_t sampleCount{1};
    Vector3U extent{};
    std::vector<uint32_t> queueAccess{};
};

enum class AspectMask : uint32_t {
    COLOR = 1 << 0,
    DEPTH = 1 << 1,
    STENCIL = 1 << 2,
    METADATA = 1 << 3,
    PLANE_0 = 1 << 4,
    PLANE_1 = 1 << 5,
    PLANE_2 = 1 << 6,
};
OPERABLE(AspectMask)

struct Range {
    AspectMask aspect{AspectMask::COLOR};
    uint32_t width{0};
    uint32_t height{0};
    uint32_t firstSlice{0};
    uint32_t sliceCount{0};
    uint32_t firstMip{0};
    uint32_t mipCount{0};
};

struct ImageSubresourceRange{
    AspectMask aspect{AspectMask::COLOR};
    uint32_t firstSlice{0};
    uint32_t sliceCount{0};
    uint32_t firstMip{0};
    uint32_t mipCount{0};
};

enum class ImageViewType : uint8_t {
    IMAGE_VIEW_1D,
    IMAGE_VIEW_2D,
    IMAGE_VIEW_3D,
    IMAGE_VIEW_CUBE,
    IMAGE_VIEW_1D_ARRAY,
    IMAGE_VIEW_2D_ARRAY,
    IMAGE_VIEW_CUBE_ARRAY,
};

enum class ComponentSwizzle : uint8_t {
    IDENTITY,
    ZERO,
    ONE,
    R,
    G,
    B,
    A,
};

struct ComponentMapping {
    ComponentSwizzle r{ComponentSwizzle::IDENTITY};
    ComponentSwizzle g{ComponentSwizzle::IDENTITY};
    ComponentSwizzle b{ComponentSwizzle::IDENTITY};
    ComponentSwizzle a{ComponentSwizzle::IDENTITY};
};

struct ImageViewInfo {
    ImageViewType type{ImageViewType::IMAGE_VIEW_2D};
    RHIImage* image{nullptr};
    ImageSubresourceRange range{};
    ComponentMapping componentMapping{};
    AspectMask aspectMask{AspectMask::COLOR};
    Format format{Format::UNKNOWN};
};

enum class InputRate : uint8_t {
    PER_VERTEX,
    PER_INSTANCE,
};

struct VertexAttribute {
    uint32_t location{0};
    uint32_t binding{0};
    Format format{Format::UNKNOWN};
    uint32_t offset{0};
};

struct VertexBufferAttribute {
    uint32_t binding{0};
    uint32_t stride{0};
    InputRate rate{InputRate::PER_VERTEX};
};

using VertexBufferAttributes = std::vector<VertexBufferAttribute>;
using VertexAttributes = std::vector<VertexAttribute>;

struct VertexLayout {
    VertexAttributes vertexAttrs;
    VertexBufferAttributes vertexBufferAttrs;
};
// struct GraphicsPipelineLayout {
//     VertexLayout vertexLayout;
// };

enum class DescriptorType {
    SAMPLER,
    SAMPLED_IMAGE,
    STORAGE_IMAGE,
    UNIFORM_TEXEL_BUFFER,
    STORAGE_TEXEL_BUFFER,
    UNIFORM_BUFFER,
    STORAGE_BUFFER,
    UNIFORM_BUFFER_DYNAMIC,
    STORAGE_BUFFER_DYNAMIC,
    INPUT_ATTACHMENT,
};

struct DescriptorBinding {
    uint32_t binding{0};
    DescriptorType type{DescriptorType::UNIFORM_BUFFER};
    uint32_t count{1};
    ShaderStage visibility;
};

using DescriptorBindings = std::vector<DescriptorBinding>;
struct DescriptorSetLayoutInfo {
    DescriptorBindings descriptorBindings;
};

struct DescriptorSetInfo {
};

struct PushConstantRange {
    ShaderStage stage{ShaderStage::VERTEX};
    uint32_t offset{0};
    uint32_t size{0};
};

struct PipelineLayoutInfo {
    std::vector<PushConstantRange> pushConstantRanges;
    std::vector<RHIDescriptorSetLayout*> setLayouts;
};

enum class LoadOp : uint8_t {
    LOAD,
    CLEAR,
    DISCARD,
};

enum class StoreOp : uint8_t {
    STORE,
    CLEAR,
};

struct AttachmentInfo {
    LoadOp loadOp{LoadOp::CLEAR};
    StoreOp storeOp{StoreOp::STORE};
    LoadOp stencilLoadOp{LoadOp::CLEAR};
    StoreOp stencilStoreOp{StoreOp::STORE};
    Format format{Format::UNKNOWN};
    uint32_t sampleCount{1};
    ImageLayout initialLayout{ImageLayout::UNDEFINED};
    ImageLayout finalLayout{ImageLayout::UNDEFINED};
};

struct AttachmentReference {
    uint8_t index;
    ImageLayout layout{ImageLayout::READ_ONLY_OPTIMAL};
};

struct SubpassInfo {
    std::vector<uint32_t> preserves;
    std::vector<AttachmentReference> inputs;
    std::vector<AttachmentReference> colors;
    std::vector<AttachmentReference> resolves;
    std::vector<AttachmentReference> depthStencil; // expect only one
};

enum class PipelineStage : uint8_t {
    TOP_OF_PIPE,
    DRAW_INDIRECT,
    VERTEX_INPUT,
    VERTEX_SHADER,
    FRAGMENT_SHADER,
    EARLY_FRAGMENT_TESTS,
    LATE_FRAGMENT_TESTS,
    COLOR_ATTACHMENT_OUTPUT,
    COMPUTE_SHADER,
    TRANSFER,
    BOTTOM_OF_PIPE,
    HOST,
    TASK_SHADER,
    MESH_SHADER,
};

enum class AccessFlags : uint32_t {
    NONE = 0,
    INDIRECT_COMMAND_READ = 1 << 0,
    INDEX_READ = 1 << 1,
    VERTEX_ATTRIBUTE_READ = 1 << 2,
    UNIFORM_READ = 1 << 3,
    INPUT_ATTACHMENT_READ = 1 << 4,
    SHADER_READ = 1 << 5,
    SHADER_WRITE = 1 << 6,
    COLOR_ATTACHMENT_READ = 1 << 7,
    COLOR_ATTACHMENT_WRITE = 1 << 8,
    DEPTH_STENCIL_ATTACHMENT_READ = 1 << 9,
    DEPTH_STENCIL_ATTACHMENT_WRITE = 1 << 10,
    TRANSFER_READ = 1 << 11,
    TRANSFER_WRITE = 1 << 12,
    HOST_READ = 1 << 13,
    HOST_WRITE = 1 << 14,
    MEMORY_READ = 1 << 15,
    MEMORY_WRITE = 1 << 16,

    SHADING_RATE_ATTACHMENT_READ = 1 << 23,
};
OPERABLE(AccessFlags)

enum class DependencyFlags : uint32_t {
    BY_REGION = 1 << 0,
    VIEW_LOCAL = 1 << 1,
    DEVICE_GROUP = 1 << 2,
    FEEDBACK_LOOP = 1 << 3,
};
OPERABLE(DependencyFlags)

struct SubpassDependency {
    uint32_t src{0xFFFFFFFF};
    uint32_t dst{0xFFFFFFFF};
    PipelineStage srcStage{PipelineStage::TOP_OF_PIPE};
    PipelineStage dstStage{PipelineStage::BOTTOM_OF_PIPE};
    AccessFlags srcAccessFlags{AccessFlags::NONE};
    AccessFlags dstAccessFlags{AccessFlags::NONE};
    DependencyFlags dependencyFlags{DependencyFlags::BY_REGION};
};

struct RenderPassInfo {
    std::vector<AttachmentInfo> attachments;
    std::vector<SubpassInfo> subpasses;
    std::vector<SubpassDependency> dependencies;
};

struct FrameBufferInfo {
    RHIRenderPass* renderPass{nullptr};
    std::vector<RHIImageView*> images{};
    uint32_t width{0};
    uint32_t height{0};
    uint32_t layers{0};
};

enum class IAType : uint8_t {
    VB_IB,
    MESH,
};

struct Rect2D {
    int32_t x{0};
    int32_t y{0};
    uint32_t w{0};
    uint32_t h{0};
};

struct Viewport {
    Rect2D rect{};
    float minDepth{0.0f};
    float maxDepth{0.0f};
};

enum class PolygonMode : uint8_t {
    FILL,
    LINE,
    POINT,
};

enum class PrimitiveType : uint8_t {
    POINT_LIST,
    LINE_LIST,
    LINE_STRIP,
    TRIANGLE_LIST,
    TRIANGLE_STRIP,
};

enum class FaceMode : uint8_t {
    NONE,
    FRONT,
    BACK,
    FRONT_AND_BACK,
};

enum class FrontFace : uint8_t {
    COUNTER_CLOCKWISE,
    CLOCKWISE,
};

struct RasterizationInfo {
    bool depthClamp{false};
    bool depthBiasEnable{false};
    PolygonMode polygonMode{PolygonMode::FILL};
    FaceMode cullMode{FaceMode::NONE};
    FrontFace frontFace{FrontFace::COUNTER_CLOCKWISE};
    float depthBiasConstantFactor{0.0f};
    float depthBiasClamp{0.0f};
    float depthBiasSlopeFactor{0.0f};
    float lineWidth{1.0f};
};

struct MultisamplingInfo {
    bool enable{false};
    bool sampleShadingEnable{false};
    bool alphaToCoverageEnable{false};
    float minSampleShading{0.0f};
    uint32_t sampleCount{0};
    uint32_t sampleMask{0xFFFFFFFF};
};

enum class CompareOp : uint8_t {
    NEVER,
    LESS,
    EQUAL,
    LESS_OR_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_OR_EQUAL,
    ALWAYS,
};

enum class StencilOp : uint8_t {
    KEEP,
    ZERO,
    REPLACE,
    INCREMENT_AND_CLAMP,
    DECREMENT_AND_CLAMP,
    INVERT,
    INCREMENT_AND_WRAP,
    DECREMENT_AND_WRAP,
};

struct StencilInfo {
    StencilOp failOp;
    StencilOp passOp;
    StencilOp depthFailOp;
    CompareOp compareOp;
    uint32_t compareMask{0};
    uint32_t writeMask{0};
    uint32_t reference{0};
};

struct DepthStencilInfo {
    bool depthTestEnable{false};
    bool depthWriteEnable{false};
    bool depthBoundsTestEnable{false};
    bool stencilTestEnable{false};
    CompareOp depthCompareOp{CompareOp::LESS_OR_EQUAL};
    StencilInfo front;
    StencilInfo back;
    float minDepthBounds{0.0f};
    float maxDepthBounds{0.0f};
};

enum class BlendFactor : uint8_t {
    ZERO,
    ONE,
    SRC_COLOR,
    ONE_MINUS_SRC_COLOR,
    DST_COLOR,
    ONE_MINUS_DST_COLOR,
    SRC_ALPHA,
    ONE_MINUS_SRC_ALPHA,
    DST_ALPHA,
    ONE_MINUS_DST_ALPHA,
    CONSTANT_COLOR,
    ONE_MINUS_CONSTANT_COLOR,
    CONSTANT_ALPHA,
    ONE_MINUS_CONSTANT_ALPHA,
    SRC_ALPHA_SATURATE,
};

enum class BlendOp : uint8_t {
    ADD,
    SUBTRACT,
    REVERSE_SUBTRACT,
    MIN,
    MAX,
};

enum class Channel : uint8_t {
    R = 1,
    G = 1 << 1,
    B = 1 << 2,
    A = 1 << 3,
};
OPERABLE(Channel)

struct AttachmentBlendInfo {
    bool blendEnable{false};
    BlendFactor srcColorBlendFactor{BlendFactor::ONE};
    BlendFactor dstColorBlendFactor{BlendFactor::ZERO};
    BlendOp colorBlendOp{BlendOp::ADD};
    BlendFactor srcAlphaBlendFactor{BlendFactor::ONE};
    BlendFactor dstAlphaBlendFactor{BlendFactor::ZERO};
    BlendOp alphaBlendOp{BlendOp::ADD};
    Channel writemask{Channel::R | Channel::G | Channel::B | Channel::A};
};

enum class LogicOp : uint8_t {
    CLEAR,
    AND,
    AND_REVERSE,
    COPY,
    AND_INVERTED,
    NOOP,
    XOR,
    OR,
    NOR,
    EQUIVALENT,
    INVERT,
    OR_REVERSE,
    COPY_INVERTED,
    OR_INVERTED,
    NAND,
    SET,
};

struct BlendInfo {
    bool logicOpEnable{false};
    LogicOp logicOp{LogicOp::CLEAR};
    std::vector<AttachmentBlendInfo> attachmentBlends;
    float blendConstants[4];
};

struct GraphicsPipelineInfo {
    PrimitiveType primitiveType{PrimitiveType::TRIANGLE_LIST};
    RHIPipelineLayout* pipelineLayout{nullptr};
    RHIRenderPass* renderPass{nullptr};
    std::vector<RHIShader*> shaders;
    uint32_t subpassIndex{0};
    uint32_t viewportCount{1};
    VertexLayout vertexLayout;
    RasterizationInfo rasterizationInfo{};
    MultisamplingInfo multisamplingInfo{};
    DepthStencilInfo depthStencilInfo{};
    BlendInfo colorBlendInfo{};
};

enum class MemoryUsage : uint8_t {
    HOST_VISIBLE,
    DEVICE_ONLY,
    STAGING,
    LAZY_ALLOCATED,
};

enum class BufferUsage : uint32_t {
    UNIFORM = 1,
    STORAGE = 1 << 1,
    INDEX = 1 << 2,
    VERTEX = 1 << 3,
    INDIRECT = 1 << 4,
    TRANSFER_SRC = 1 << 5,
    TRANSFER_DST = 1 << 6,
};
OPERABLE(BufferUsage)

enum class BufferFlag : uint32_t {
    NONE = 0,
    SPARSE_BINDING = 1 << 0,
    SPARSE_RESIDENCY = 1 << 1,
    SPARSE_ALIASED = 1 << 2,
};
OPERABLE(BufferFlag)

struct BufferInfo {
    MemoryUsage memUsage{MemoryUsage::DEVICE_ONLY};
    SharingMode sharingMode{SharingMode::EXCLUSIVE};
    BufferFlag flag{BufferFlag::NONE};
    BufferUsage bufferUsage{BufferUsage::UNIFORM};
    uint32_t size{0};
    std::vector<uint32_t> queueAccess{};
};

struct BufferViewInfo {
    RHIBuffer* buffer{nullptr};
    Format format{Format::UNKNOWN};
    uint32_t offset{0};
    uint32_t size{0};
};

enum class IndexType : uint8_t {
    HALF, // most likely 16 bit
    FULL, // most likely 32 bit
};
struct DeviceInfo {
    // void* hwnd;
};

enum class CommandBufferType : uint8_t {
    PRIMARY,
    SECONDARY,
};

struct CommandBufferInfo {
    CommandBufferType type{CommandBufferType::PRIMARY};
};

struct BufferCopyRegion {
    uint32_t srcOffset{0};
    uint32_t dstOffset{0};
    uint32_t size{0};
};

struct ImageCopyRegion {
    AspectMask srcImageAspect{AspectMask::COLOR};
    AspectMask dstImageAspect{AspectMask::COLOR};
    Vector3U srcOffset;
    Vector3U dstOffset;
    Vector3U extent;
    uint32_t srcBaseMip{0};
    uint32_t srcFirstSlice{0};
    uint32_t dstBaseMip{0};
    uint32_t dstFirstSlice{0};
    uint32_t sliceCount{0};
};

struct ImageBlit {
    AspectMask srcImageAspect{AspectMask::COLOR};
    AspectMask dstImageAspect{AspectMask::COLOR};
    Vector3U srcOffset;
    Vector3U dstOffset;
    uint32_t srcBaseMip{0};
    uint32_t srcFirstSlice{0};
    uint32_t dstBaseMip{0};
    uint32_t dstFirstSlice{0};
    uint32_t sliceCount{0};
    Vector3U srcExtent;
    Vector3U dstExtent;
};

struct ImageResolve {
    AspectMask srcImageAspect{AspectMask::COLOR};
    AspectMask dstImageAspect{AspectMask::COLOR};
    Vector3U srcOffset;
    Vector3U dstOffset;
    uint32_t srcBaseMip{0};
    uint32_t srcFirstSlice{0};
    uint32_t dstBaseMip{0};
    uint32_t dstFirstSlice{0};
    uint32_t sliceCount{0};
    Vector3U extent{};
};

struct BufferImageCopyRegion {
    uint32_t bufferSize{0};
    uint32_t bufferOffset{0};
    uint32_t bufferRowLength{0};
    uint32_t bufferImageHeight{0};
    AspectMask imageAspect{AspectMask::COLOR};
    uint32_t baseMip{0};
    uint32_t firstSlice{0};
    uint32_t sliceCount{0};
    Vector3U imageOffset{};
    Vector3U imageExtent{};
};

enum class Filter : uint8_t {
    NEAREST,
    LINEAR,
    CUBIC,
};

struct ClearRect {
    int32_t x{0};
    int32_t y{0};
    uint32_t width{0};
    uint32_t height{0};
    uint32_t firstSlice{0};
    uint32_t sliceCount{0};
};

enum class ElementType : uint8_t {
    FLOAT,
    SINT,
    UINT,
};

union ClearColor {
    float clearColorF[4];
    uint32_t clearColorU[4];
    int32_t clearColorI[4];
    float depth;
    uint32_t stencil;
};

struct RenderPassBeginInfo {
    RHIRenderPass* renderPass{nullptr};
    RHIFrameBuffer* frameBuffer{nullptr};
    Rect2D renderArea;
    ClearColor* clearColors;
};

struct ImageBarrierInfo {
    RHIImage* image{nullptr};
    PipelineStage srcStage{PipelineStage::TOP_OF_PIPE};
    PipelineStage dstStage{PipelineStage::BOTTOM_OF_PIPE};
    ImageLayout oldLayout{ImageLayout::UNDEFINED};
    ImageLayout newLayout{ImageLayout::UNDEFINED};
    AccessFlags srcAccessFlag{AccessFlags::NONE};
    AccessFlags dstAccessFlag{AccessFlags::NONE};
    uint32_t srcQueueIndex{0};
    uint32_t dstQueueIndex{0};
    ImageSubresourceRange range;
};

struct BufferBarrierInfo {
    RHIBuffer* buffer{nullptr};
    PipelineStage srcStage{PipelineStage::TOP_OF_PIPE};
    PipelineStage dstStage{PipelineStage::BOTTOM_OF_PIPE};
    AccessFlags srcAccessFlag{AccessFlags::NONE};
    AccessFlags dstAccessFlag{AccessFlags::NONE};
    uint32_t srcQueueIndex{0};
    uint32_t dstQueueIndex{0};
    uint32_t offset{0};
    uint32_t size{0};
};

enum class CommandBuferUsageFlag : uint8_t {
    ONE_TIME_SUBMIT = 1,
    RENDER_PASS_CONTINUE = 1 << 2,
    SIMULTANEOUS_USE = 1 << 3,
};

struct CommandBufferBeginInfo {
    CommandBuferUsageFlag flags{CommandBuferUsageFlag ::ONE_TIME_SUBMIT};
};
OPERABLE(CommandBuferUsageFlag)

} // namespace raum::rhi