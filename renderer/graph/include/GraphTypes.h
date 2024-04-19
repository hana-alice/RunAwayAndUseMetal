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
using ShaderStage = rhi::ShaderStage;
using BufferUsage = rhi::BufferUsage;

struct RenderingResource {
    std::string name{};
    std::string bindingName{};
    Access access{Access::READ};
    ShaderStage visibility{ShaderStage::FRAGMENT};
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
    rhi::Rect2D renderArea;
};

struct SubRenderPassData {
    std::vector<AttachmentResource> attachments;
};

struct RenderQueueData {
    scene::Camera* camera{nullptr};
    rhi::Viewport viewport{};
    std::vector<RenderingResource> resources;
    scene::BindGroupPtr bindGroup;
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
    std::vector<uint8_t> data;
    uint32_t size{0};
    std::uint32_t offset{0};
    std::string name;
};

struct CopyPassData {
    std::vector<CopyPair> copies;
    std::vector<UploadPair> uploads;
};

struct BufferData {
    rhi::BufferInfo info{};
    rhi::BufferPtr buffer;
};

struct BufferViewData {
    std::string origin{};
    rhi::BufferViewInfo info{};
    rhi::BufferViewPtr bufferView;
};

struct ImageData {
    rhi::ImageInfo info{};
    rhi::ImagePtr image;
};

struct ImageViewData {
    std::string origin{};
    rhi::ImageViewInfo info{};
    rhi::ImageViewPtr imageView;
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

enum class Rate {
    PER_PASS,
    PER_BATCH,
    PER_INSTANCE,
    PER_DRAW,
};

struct ShaderBindingDesc {
    BindingType type{BindingType::BUFFER};
    rhi::ShaderStage visibility{rhi::ShaderStage::NONE};
    Rate rate{Rate::PER_PASS};
    uint32_t binding{0};
    BufferBinding buffer{};
    ImageBinding image{};
    SamplerBinding sampler{};
};

struct ShaderResource {
    std::unordered_map<std::string, ShaderBindingDesc, hash_string, std::equal_to<>> bindings;
    std::array<rhi::DescriptorSetLayoutPtr, rhi::BindingRateCount> descriptorLayouts;
    rhi::PipelineLayoutPtr pipelineLayout;
    boost::container::flat_map<std::string, std::string> shaderSources;
    boost::container::flat_map<rhi::ShaderStage, rhi::ShaderPtr> shaders;
};

using ShaderResources = std::unordered_map<std::string, ShaderResource, hash_string, std::equal_to<>>;

enum class RenderQueueFlags: uint32_t {
    NONE = 0,
    OPAQUE = 1,
    TRANSPARENT = 1 << 1,
};

} // namespace raum::graph