#include "SceneGraph.h"

using boost::add_vertex;
using boost::get;
using boost::vertices;

namespace raum::graph {


scene::Model&  SceneGraph::addModel(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = scene::Model();
    return std::get<scene::Model>(_graph[id].sceneNodeData);
}



}