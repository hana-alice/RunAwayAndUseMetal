#include "ShaderGraph.h"
#include "RHIDescriptorSetLayout.h"
#include "RHIDescriptorPool.h"
#include <fstream>
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include "RHIDefine.h"
#include "RHIUtils.h"
#include "Serialization.h"
#include "boost/graph/depth_first_search.hpp"
#include "RHIDevice.h"

using boost::add_vertex;
using boost::graph::find_vertex;
using boost::add_edge;

namespace raum::graph {

void parseName(const std::filesystem::path &logicPath, ShaderGraph& sg) {
    auto& graph = sg.underlyingGraph();

    auto currPath = logicPath;
    while(currPath.empty()) {
        const auto& parent = currPath.parent_path().string();
        const auto& current = currPath.string();
        add_vertex(parent, graph);
        add_vertex(current, graph);

        add_edge(parent, current, graph);
        currPath = currPath.parent_path();
    }
}

std::map<std::string, std::string> loadResource(std::filesystem::path dir, std::string_view name) {
    std::map<std::string, std::string> shaderSrc;
    for(auto ext : {".vert", ".frag", ".comp", ".mesh", ".task"}) {
        auto fp = (dir / name).concat(ext);
        if(exists(fp)) {
            std::ifstream in(fp);
            std::stringstream buffer;
            buffer << in.rdbuf();
            shaderSrc.emplace(ext, buffer.str());
        }
    }
    return shaderSrc;
}

void reflect(const std::string& source, std::map<uint32_t, std::string>& bindingMap) {
    const char* pattern = R"(\s*layout\([^\)]*binding\s*=\s(\d+)\).*?(\w+)\s*[{;])";
    boost::regex reg(pattern);

    boost::sregex_iterator it(source.begin(), source.end(), reg);
    boost::sregex_iterator end;

    for (; it != end; ++it) {
        std::string matchStr((*it)[0].begin(), (*it)[0].end());
        const char* setPatt = R"(set\s*=\s*0)";
        boost::regex setReg(setPatt);
        boost::sregex_iterator setIt(matchStr.begin(), matchStr.end(), setReg);
        if (setIt == end) continue; // skip if not set 0

        std::string_view bindingStr((*it)[1].begin(), (*it)[1].end());
        std::string_view nameStr((*it)[2].begin(), (*it)[2].end());
        auto binding = boost::lexical_cast<uint32_t>(bindingStr);
        bindingMap.emplace(binding, nameStr);
    }
}

std::map<uint32_t, std::string> reflect(const std::map<std::string, std::string>& sources) {
    std::map<uint32_t, std::string> bindingMap;
    for(auto source : sources) {
        reflect(source.second, bindingMap);
    }
    return bindingMap;
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
    }

    ShaderResources& resources;
    rhi::RHIDevice* device{nullptr};
};

}


void ShaderGraph::load(const std::filesystem::path &path, std::string_view name) {
    auto layoutPath = (path / name ).concat(".layout");
    assert(exists(layoutPath));

    auto shaderSrc = loadResource(path, name);
    const auto& bindingMap = reflect(shaderSrc);
    const auto& logicPath = deserialize(layoutPath, _resources, bindingMap);
    parseName(logicPath, *this);

    for(auto& src : shaderSrc) {
        _resources[logicPath.string()].shaderSources.emplace(logicPath.string() + src.first, std::move(src.second));
    }
}

void ShaderGraph::registerCustomLayout(ShaderResource&& layout, std::string_view name) {
    raum_check(!_resources.contains("name"), "name has already been used.");
    _resources.emplace(name, std::move(layout));
}

void ShaderGraph::compile(std::string_view name) {
    const auto& vert = *find_vertex(name.data(), _impl);
    ShaderVisitor visitor{{}, _resources, _device};

    auto indexMap = boost::get(boost::vertex_index, _impl);
    auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);

    boost::depth_first_visit(_impl, vert, visitor, colorMap);
}

rhi::RHIDescriptorSetLayout* ShaderGraph::getLayout(std::string_view name) {
    return nullptr;
}

}