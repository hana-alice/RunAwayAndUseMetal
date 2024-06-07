#include "SceneGraph.h"

using boost::add_vertex;
using boost::get;
using boost::vertices;
using boost::graph::find_vertex;

namespace raum::graph {


ModelNode& SceneGraph::addModel(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = ModelNode{};
    return std::get<ModelNode>(_graph[id].sceneNodeData);
}

CameraNode& SceneGraph::addCamera(std::string_view name, std::string_view parent, const scene::Frustum& frustum, scene::Projection proj) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = CameraNode{};
    return std::get<CameraNode>(_graph[id].sceneNodeData);
}

LightNode& SceneGraph::addLight(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = LightNode{};
    return std::get<LightNode>(_graph[id].sceneNodeData);
}

void SceneGraph::enable(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    _graph[v].enable = true;
}

void SceneGraph::disable(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    _graph[v].enable = false;
}

}