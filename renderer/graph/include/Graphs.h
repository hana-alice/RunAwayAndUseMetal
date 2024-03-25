#pragma once
#include "ResourceGraph.h"
#include "RenderGraph.h"
#include "ShaderGraph.h"
#include "TaskGraph.h"
#include "AccessGraph.h"
#include "SceneGraph.h"

namespace raum::graph {

void execute(RenderGraph& rg, AccessGraph& ag, ResourceGraph& resg, ShaderGraph& shg, SceneGraph& sg, TaskGraph& tg);

}
