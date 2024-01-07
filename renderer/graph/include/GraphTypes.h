#pragma once
#include <string>
#include <variant>
#include <vector>
#include "RHIDefine.h"
namespace raum::graph {

enum class Access : uint8_t {
    READ,
    WRITE,
    READ_WRITE,
};

struct RenderingResource {
    std::string name{};
    std::string bindingName{};
    Access access{Access::READ};
};

struct RenderPass {
    std::string name{};
    std::vector<RenderingResource> resources;
};

struct SubRenderPass {
    std::string name{};
    std::vector<RenderingResource> resources;
};

struct ComputePass {
    std::string name{};
    std::vector<RenderingResource> resources;
};

struct CopyPair {
    std::string source{};
    std::string target{};
    std::variant<rhi::BufferCopyRegion, rhi::BufferImageCopyRegion, rhi::ImageBlit, rhi::ImageCopyRegion> region;
};

struct CopyPass {
    std::string name{};
    std::vector<CopyPair> pairs;
};

} // namespace raum::graph