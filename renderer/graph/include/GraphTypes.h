#pragma once
#include <string>
#include <variant>
#include <vector>
#include "boost/container/flat_map.hpp"
#include "RHIDefine.h"
#include "Camera.h"
#include "Scene.h"
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
    std::vector<RenderingResource> attachments;
};

struct SubRenderPassData {
    std::vector<RenderingResource> attachments;
};

struct RenderQueueData {
    scene::Camera* camera{nullptr};
    scene::Scene* scene{nullptr};
    rhi::Viewport viewport{};
    std::string phase{};
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

struct BufferElement {
    rhi::DataType type{rhi::DataType::UNKNOWN};
    uint32_t count{1};
};

struct BufferBinding {
    rhi::DescriptorType type{rhi::DescriptorType::UNIFORM_BUFFER};
    std::vector<BufferElement> elements;
    uint32_t count{1};
};

struct ImageBinding {
    rhi::DescriptorType type{rhi::DescriptorType::SAMPLED_IMAGE};
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
    rhi::RHIDescriptorSetLayout* layout{nullptr};
    boost::container::flat_map<std::string, std::string> shaderSources;
    boost::container::flat_map<rhi::ShaderStage, rhi::RHIShader*> shaders;
};

using ShaderResources = std::unordered_map<std::string, ShaderResource, hash_string, std::equal_to<>>;

enum class RenderQueueFlags: uint32_t {
    NONE = 0,
    OPAQUE = 1,
    TRANSPARENT = 1 << 1,
};

} // namespace raum::graph