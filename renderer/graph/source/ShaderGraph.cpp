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

namespace {

std::unordered_map<rhi::DescriptorSetLayoutInfo, rhi::DescriptorSetLayoutRef, rhi::RHIHash<rhi::DescriptorSetLayoutInfo>> _descriptorsetLayoutMap;
std::unordered_map<rhi::PipelineLayoutInfo, rhi::PipelineLayoutRef, rhi::RHIHash<rhi::PipelineLayoutInfo>> _pplLayoutMap;

rhi::DescriptorSetLayoutPtr getOrCreateDescriptorSetLayout(const rhi::DescriptorSetLayoutInfo& info, rhi::DevicePtr device) {
    rhi::DescriptorSetLayoutPtr res;
    if(!_descriptorsetLayoutMap.contains(info) || _descriptorsetLayoutMap.at(info).expired()) {
        res = rhi::DescriptorSetLayoutPtr(device->createDescriptorSetLayout(info));
        _descriptorsetLayoutMap[info] = res;
    } else {
        res = _descriptorsetLayoutMap[info].lock();
    }
    return res;
}

rhi::PipelineLayoutPtr getOrCreatePipelineLayout(const rhi::PipelineLayoutInfo& info, rhi::DevicePtr device) {
    rhi::PipelineLayoutPtr res;
    if(!_pplLayoutMap.contains(info) || _pplLayoutMap.at(info).expired()) {
        res = rhi::PipelineLayoutPtr(device->createPipelineLayout(info));
        _pplLayoutMap[info] = res;
    } else {
        res = _pplLayoutMap[info].lock();
    }
    return res;
}

}


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

ShaderGraph::ShaderGraph(rhi::DevicePtr device):_device(device) {
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

void generateDescriptorSetLayouts(ShaderResource& resource, rhi::DevicePtr device) {
    std::array<rhi::DescriptorSetLayoutInfo, BindingRateCount> infos;
    auto& layouts = resource.descriptorLayouts;
    for(const auto& binding : resource.bindings) {
        const auto& bindingDesc = binding.second;
        auto index = static_cast<uint32_t>(bindingDesc.rate);
        rhi::DescriptorSetLayoutPtr& layout = layouts[index];
        rhi::DescriptorType type{rhi::DescriptorType::UNIFORM_BUFFER};
        uint32_t count{1};
        switch (bindingDesc.type) {
            case BindingType::BUFFER:
                type = bindingDesc.buffer.type;
                count = bindingDesc.buffer.count;
                break;
            case BindingType::IMAGE:
                type = bindingDesc.image.type;
                count = bindingDesc.image.arraySize;
                break;
            case BindingType::SAMPLER:
                type = rhi::DescriptorType::SAMPLER;
        }
        infos[index].descriptorBindings.emplace_back(bindingDesc.binding, type, count, bindingDesc.visibility, std::vector<rhi::RHISampler*>());
    }

    std::vector<rhi::RHIDescriptorSetLayout*> descriptors(BindingRateCount);
    for (size_t i = 0; i < BindingRateCount; ++i) {
        layouts[i] = getOrCreateDescriptorSetLayout(infos[i], device);
        descriptors[i] = layouts[i].get();
    }

    resource.pipelineLayout = getOrCreatePipelineLayout({{}, descriptors}, device);
}

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
            auto shaderPtr = rhi::ShaderPtr(device->createShader(info));
            resource.shaders.emplace(stage, shaderPtr);
        }

        generateDescriptorSetLayouts(resource, device);
    }

    ShaderResources& resources;
    rhi::DevicePtr device{nullptr};
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

//rhi::DescriptorSetLayoutInfo ShaderGraph::layoutInfo(std::string_view name, Rate rate) {
//    return _resources.at(name.data()).descriptorLayouts[static_cast<uint32_t>(rate)];
//}

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