#pragma once
#include "RenderGraph.h"

namespace raum::graph {


class AccessGraph {
public:
    AccessGraph() = delete;
    AccessGraph(RenderGraph& rg);

    void analyze();

private:
    RenderGraph& _rg;
};

}