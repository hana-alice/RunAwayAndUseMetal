#pragma once
#include "RenderGraph.h"
#include "ResourceGraph.h"
#include "ShaderGraph.h"

namespace raum::graph {

class AccessGraph {
public:
    struct Access {
        RenderGraph::VertexType v{0};
        rhi::AccessFlags access{rhi::AccessFlags::NONE};
    };
    using ResourceAccessMap = std::unordered_map<std::string_view, std::vector<Access>, hash_string, std::equal_to<>>;

    AccessGraph() = delete;
    AccessGraph(RenderGraph& rg, ResourceGraph& resg, ShaderGraph& sg);

    void analyze();

private:
    RenderGraph& _rg;
    ResourceGraph& _resg;
    ShaderGraph& _sg;
    ResourceAccessMap _accessMap;
};

}