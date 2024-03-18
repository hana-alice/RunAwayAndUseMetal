#include "AccessGraph.h"

namespace raum::graph {

namespace {

class AccessVisitor{
public:
    AccessVisitor(AccessGraph& agIn, RenderGraph& rgIn):ag(agIn), rg(rgIn) {}

    void discover_vertex() {

    }

    AccessGraph& ag;
    RenderGraph& rg;
};

}

AccessGraph::AccessGraph(raum::graph::RenderGraph &rg): _rg(rg) {}

void AccessGraph::analyze() {
}


}