#include "ShaderGraph.h"
#include "RHIDescriptorSetLayout.h"
#include "RHIDescriptorPool.h"
#include <fstream>
#include "boost/regex.hpp"
#include "boost/lexical_cast.hpp"
#include "RHIDefine.h"
#include "RHIUtils.h"
#include "Serialization.h"

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


ShaderGraph::ShaderGraph(rhi::RHIDevice *device):_device(device) {
    auto v = add_vertex("", _impl);
}


void ShaderGraph::load(const std::filesystem::path &path, std::string_view name) {
    auto layoutPath = (path / name ).concat(".layout");

    assert(exists(layoutPath));

    const auto& bindingMap = reflect(path, name);
    deserialize(layoutPath, _resources, bindingMap);
}

rhi::RHIDescriptorSetLayout* getLayout(std::string_view name) {
    return nullptr;
}

}