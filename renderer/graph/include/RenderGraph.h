#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "GraphTypes.h"
namespace raum::graph {

using Pass = std::variant<RenderPass, SubRenderPass, ComputePass, CopyPass>;

} // namespace raum::graph