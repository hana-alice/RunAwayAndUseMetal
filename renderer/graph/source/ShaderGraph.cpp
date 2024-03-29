#include "ShaderGraph.h"
#include "RHIDescriptorSetLayout.h"
#include "RHIDescriptorPool.h"
#include <fstream>

#include "RHIDefine.h"
#include "RHIUtils.h"
#include "Serialization.h"
#include "boost/graph/depth_first_search.hpp"
#include "RHIDevice.h"

using boost::add_vertex;
using boost::graph::find_vertex;
using boost::add_edge;

namespace raum::graph {

void ShaderGraph::addVertex(const std::filesystem::path &logicPath, ShaderResource&& shaderResource) {
    auto& graph = _impl;

    auto currPath = logicPath;
    while (!currPath.empty()) {
        const auto& parent = currPath.parent_path().string();
        const auto& current = currPath.string();
        add_vertex(parent, graph);
        add_vertex(current, graph);

        add_edge(parent, current, graph);
        currPath = currPath.parent_path();
    }

    _resources.emplace(logicPath.string(), std::forward<ShaderResource>(shaderResource));
}

ShaderGraph::ShaderGraph(rhi::RHIDevice *device):_device(device) {
    auto v = add_vertex("", _impl);
}

namespace {

std::unordered_map<std::string_view, rhi::ShaderStage> str2ShaderStage = {
    {".vert", rhi::ShaderStage::VERTEX},
    {".frag", rhi::ShaderStage::FRAGMENT},
    {".comp", rhi::ShaderStage::COMPUTE},
    {".mesh", rhi::ShaderStage::MESH},
    {".task", rhi::ShaderStage::TASK},
};

struct ShaderVisitor: public boost::dfs_visitor<>{
    void discover_vertex(const ShaderGraphImpl::vertex_descriptor u, const ShaderGraphImpl& g) {
        const auto& name = g[u].name;

        if (!resources.contains(name)) {
            return;
        }

        auto& resource = resources.at(name);
        for(const auto& [idName, src] : resource.shaderSources) {
            std::string_view ext(&idName[idName.length() - 5], 5);
            auto stage  = str2ShaderStage.at(ext);
            rhi::ShaderSourceInfo info{
                idName,
                {
                    stage,
                    src,
                }
            };
            auto* shader = device->createShader(info);
            resource.shaders.emplace(stage, shader);
        }

        rhi::DescriptorSetLayoutInfo info{};
        for (const auto& [_, binding] : resource.bindings) {
            info.descriptorBindings.emplace_back();
        }
    }

    ShaderResources& resources;
    rhi::RHIDevice* device{nullptr};
};

}

void ShaderGraph::compile(std::string_view name) {
    auto pVert = find_vertex(name.data(), _impl);
    if (!pVert) {
        error("name not found in shaderGraph");
        return;
    }

    const auto& vert = *pVert;
    ShaderVisitor visitor{{}, _resources, _device};

    auto indexMap = boost::get(boost::vertex_index, _impl);
    auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);

    boost::depth_first_visit(_impl, vert, visitor, colorMap);
}

rhi::RHIDescriptorSetLayout* ShaderGraph::getLayout(std::string_view name) {
    return nullptr;
}

void ShaderGraph::addCustomLayout(ShaderResource&& layout, std::string_view name) {
    if(_resources.contains(name)) {
        error("name already exists!");
    }
    _resources.emplace(name, std::forward<ShaderResource>(layout));
}

const ShaderResource& ShaderGraph::layout(std::string_view name) const {
    return _resources.at(name.data());
}

}