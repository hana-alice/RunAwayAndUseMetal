#include "ShaderGraph.h"
#include "RHIDescriptorSetLayout.h"
#include "RHIDescriptorPool.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"

#include <iostream>

using boost::add_vertex;
using boost::graph::find_vertex;
using boost::add_edge;

using json = nlohmann::json;

//json type conversions
namespace {
//NLOHMANN_JSON_SERIALIZE_ENUM( TaskState, {
//                                            {TS_INVALID, nullptr},
//                                            {TS_STOPPED, "stopped"},
//                                            {TS_RUNNING, "running"},
//                                            {TS_COMPLETED, "completed"},
//                                        })


}


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


void reflect(const std::string& source, std::map<uint32_t, std::string>& bindingMap) {
    const char* pattern = R"(layout\(.*?binding\s*=\s(\d+)\).*?(\w+)\s*[{;])";
    boost::regex reg(pattern);

    boost::sregex_iterator it(source.begin(), source.end(), reg);
    boost::sregex_iterator end;

    for (; it != end; ++it) {
        std::string_view bindingStr((*it)[1].begin(), (*it)[1].end());
        std::string_view nameStr((*it)[2].begin(), (*it)[2].end());
        auto binding = boost::lexical_cast<uint32_t>(bindingStr);
        bindingMap.emplace(binding, nameStr);
    }
}

std::map<uint32_t, std::string> reflect(std::filesystem::path dir, std::string_view name) {
    std::map<uint32_t, std::string> bindingMap;

    for(auto ext : {".vert", ".frag", ".comp", ".mesh", ".task"}) {
        auto fp = (dir / name).concat(ext);
        if(exists(fp)) {
            std::ifstream in(fp);
            std::stringstream buffer;
            buffer << in.rdbuf();
            reflect(buffer.str(), bindingMap);
        }
    }
    return bindingMap;
}

void parseBindings(const json& data, ShaderResource& resource) {
    if(data.contains("vertex")) {
        const auto& vertexBindings = data["vertex"];

        //assert(vertexBindings.is_array());
        for(const auto& binding : vertexBindings) {
            const auto& v = binding;
            //            binding.va
        }
    }

}

ShaderGraph::ShaderGraph(rhi::RHIDevice *device):_device(device) {
    auto v = add_vertex("", _impl);
}


void ShaderGraph::load(const std::filesystem::path &path, std::string_view name) {
    auto layoutPath = (path / name ).concat(".layout");

    auto bindingMap = reflect(path, name);

    assert(exists(layoutPath));
    ShaderResourceNode node;
    std::ifstream f(layoutPath);
    json data = json::parse(f);
    data.at("path").get_to(node.name);


    // add node
    auto logicPath = std::filesystem::path(node.name);
    parseName(logicPath, *this);
    parseBindings(data, _resources[node.name]);




}

rhi::RHIDescriptorSetLayout* getLayout(std::string_view name) {
    return nullptr;
}

}