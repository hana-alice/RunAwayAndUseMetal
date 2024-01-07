#pragma once
#include <string>
#include <variant>
#include <vector>
#include "RHIDefine.h"
namespace raum::graph {

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
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
    std::string name{};
    std::vector<RenderingResource> resources;
};

struct SubRenderPassData {
    std::string name{};
    std::vector<RenderingResource> resources;
};

struct ComputePassData {
    std::string name{};
    std::vector<RenderingResource> resources;
};

struct CopyPair {
    std::string source{};
    std::string target{};
    std::variant<rhi::BufferCopyRegion, rhi::BufferImageCopyRegion, rhi::ImageBlit, rhi::ImageCopyRegion> region;
};

struct CopyPassData {
    std::string name{};
    std::vector<CopyPair> pairs;
};

} // namespace raum::graph