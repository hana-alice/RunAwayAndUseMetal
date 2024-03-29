#pragma once
#include <string>
#include <variant>
#include <vector>
#include "boost/container/flat_map.hpp"
#include "RHIDefine.h"
#include "Camera.h"
#include "Scene.h"
#include <map>
namespace raum::graph {

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

constexpr size_t INVALID_VERTEX = 0xFFFFFFFF;

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

using LoadOp = rhi::LoadOp;
using StoreOp = rhi::StoreOp;
using ClearValue = rhi::ClearValue;

struct RenderingResource {
    std::string name{};
    std::string bindingName{};
    Access access{Access::READ};
    ResourceType type{ResourceType::COLOR};
};

struct AttachmentResource {
    std::string name{};
    std::string bindingName{};
    ClearValue clearValue;
    Access access{Access::READ};
    ResourceType type{ResourceType::COLOR};
    LoadOp loadOp{LoadOp::DONT_CARE};
    StoreOp storeOp{StoreOp::DONT_CARE};
    LoadOp stencilLoadOp{LoadOp::DONT_CARE};
    StoreOp stencilStoreOp{StoreOp::DONT_CARE};
};

struct RenderPassData {
    std::vector<AttachmentResource> attachments;
    rhi::RenderPassPtr renderpass;
    rhi::FrameBufferPtr framebuffer;
};

struct SubRenderPassData {
    std::vector<AttachmentResource> attachments;
};

struct RenderQueueData {
    scene::Camera* camera{nullptr};
    rhi::Viewport viewport{};
    std::string phase{};
    std::map<uint32_t, scene::RenderablePtr> renderables;
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

struct UploadPair {
    uint8_t* data{nullptr};
    uint32_t size{0};
    std::uint32_t offset{0};
    std::string_view name;
};

struct CopyPassData {
    std::vector<CopyPair> copies;
    std::vector<UploadPair> uploads;
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