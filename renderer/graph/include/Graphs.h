#pragma once
#include "ResourceGraph.h"
#include "RenderGraph.h"
#include "ShaderGraph.h"
#include "TaskGraph.h"
#include "AccessGraph.h"

namespace raum::graph {

void execute(RenderGraph& rg, AccessGraph& ag, ResourceGraph& resg, ShaderGraph& sg, TaskGraph& tg);

}
