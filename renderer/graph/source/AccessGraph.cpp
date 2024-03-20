#include "AccessGraph.h"
#include <boost/graph/depth_first_search.hpp>

namespace raum::graph {

namespace {

struct AccessVisitor : public boost::dfs_visitor<> {
public:
    void discover_vertex(const RenderGraphImpl::vertex_descriptor v, const RenderGraphImpl& rg) {
        if(std::holds_alternative<RenderPassData>(rg[v].data)) {
            auto& data = std::get<RenderPassData>(rg[v].data);
            for(const auto& res : data.resources) {
                // convention: "_" represents color/depth outputs
                if(res.bindingName == "_") {
                    rhi::AccessFlags access{rhi::AccessFlags::NONE};
                    if(res.access != Access::READ) {
//                        access |= rhi::AccessFlags::COLOR_ATTACHMENT_WRITE;
                    }
                    _accessMap[res.name].emplace_back(v, access);
                } else {

                }
            }
        }
    }
    const ResourceGraph& _resg;
    const ShaderGraph& _sg;
    AccessGraph::ResourceAccessMap& _accessMap;
};

}

AccessGraph::AccessGraph(RenderGraph &rg, ResourceGraph& resg, ShaderGraph& sg): _rg(rg), _resg(resg), _sg(sg) {}

void AccessGraph::analyze() {
    auto indexMap = boost::get(boost::vertex_index, _rg.impl());
    auto colorMap = boost::make_vector_property_map<boost::default_color_type>(indexMap);

    AccessVisitor visitor{{}, _resg, _sg, _accessMap};
    boost::depth_first_visit(_rg.impl(), 0, visitor, colorMap);
}


}