#pragma once
#include <string>
#include <variant>
#include <vector>
#include "RHIDefine.h"
namespace raum::graph {

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

enum class Access : uint8_t {
    READ,
    WRITE,
    READ_WRITE,
};

enum class ResourceType : uint8_t {
    COLOR,
    DEPTH,
    STENCIL,
    DEPTH_STENCIL,
    SHADING_RATE,
    INDIRECT_BUFFER,
};

struct RenderingResource {
    std::string name{};
    std::string bindingName{};
    Access access{Access::READ};
    ResourceType type{ResourceType::COLOR};
};

struct RenderPassData {
    std::vector<RenderingResource> resources;
};

struct SubRenderPassData {
    std::vector<RenderingResource> resources;
};

struct ComputePassData {
    std::vector<RenderingResource> resources;
};

struct CopyPair {
    std::string source{};
    std::string target{};
    std::variant<rhi::BufferCopyRegion, rhi::BufferImageCopyRegion, rhi::ImageBlit, rhi::ImageCopyRegion> region;
};

struct CopyPassData {
    std::vector<CopyPair> pairs;
};

struct BufferData {
    rhi::BufferInfo info{};
    rhi::RHIBuffer* buffer{nullptr};
};

struct BufferViewData {
    std::string origin{};
    rhi::BufferViewInfo info{};
    rhi::RHIBufferView* bufferView{nullptr};
};

struct ImageData {
    rhi::ImageInfo info{};
    rhi::RHIImage* image{nullptr};
};

struct ImageViewData {
    std::string origin{};
    rhi::ImageViewInfo info{};
    rhi::RHIImageView* imageView{nullptr};
};

enum class BindingType :uint8_t {
    BUFFER,
    IMAGE,
    SAMPLER,
    BINDLESS,
};

enum class ResourceUsage:uint8_t {
    UNIFORM,
    SAMPLED,
    STORAGE,
};

struct BufferElement {
    rhi::DataType type{rhi::DataType::UNKNOWN};
    uint32_t count{1};
};

struct BufferBinding {
    rhi::BufferUsage usage{rhi::BufferUsage::UNIFORM};
    std::vector<BufferElement> elements;
    uint32_t count{1};
};

struct ImageBinding {
    rhi::ImageUsage usage{rhi::ImageUsage::SAMPLED};
    rhi::ImageType imageType{rhi::ImageType::IMAGE_2D};
    rhi::Format format{rhi::Format::RGBA8_UNORM};
    uint32_t arraySize{1};
};

struct SamplerBinding {
    uint32_t count{1};
    bool immutable{false};
};

struct ShaderBindingDesc {
    BindingType type{BindingType::BUFFER};
    rhi::ShaderStage visibility{rhi::ShaderStage::NONE};
    uint32_t binding{0};
    BufferBinding buffer{};
    ImageBinding image{};
    SamplerBinding sampler{};
};

struct ShaderResource {
    std::unordered_map<std::string, ShaderBindingDesc, hash_string, std::equal_to<>> bindings;
    rhi::RHIDescriptorSetLayout* _layout{nullptr};
    rhi::RHIShader* _shader{nullptr};
};

} // namespace raum::graph