#include "Graphs.h"
#include <boost/graph/depth_first_search.hpp>

namespace raum::graph {

struct RenderGraphVisitor : public boost::dfs_visitor<> {
    void discover_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
         if (std::holds_alternative<RenderQueueData>(g[v].data)) {

         }
    }

    void finish_vertex(const RenderGraph::VertexType v, const RenderGraphImpl& g) {
    }
};

void execute(RenderGraph& rg, ResourceGraph& resg, ShaderGraph& sg, TaskGraph& tg) {
    AccessGraph ag{rg, resg, sg};
    ag.analyze();
}

} // namespace raum::graph