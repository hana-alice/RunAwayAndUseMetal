#pragma once

#include <filesystem>

namespace raum::rhi {
class RHIDevice;
class RHIDescriptorSetLayout;
}

namespace raum::graph {
class ShaderGraph {
public:
    ShaderGraph(rhi::RHIDevice* device);

    void load(const std::filesystem::path& path);

    rhi::RHIDescriptorSetLayout getLayout(std::string_view name);
private:
    rhi::RHIDevice* _device{nullptr};
};
}