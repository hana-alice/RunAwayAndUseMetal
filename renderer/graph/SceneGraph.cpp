#include "SceneGraph.h"


namespace raum::graph {


SceneNode& SceneGraph::addModel(std::string_view name, std::string_view parent) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = ModelNode{};
    add_edge(parent.data(), id, _graph);
    _models.emplace_back(std::ref(_graph[id]));
    return _graph[id];
}

SceneNode& SceneGraph::addCamera(std::string_view name, std::string_view parent) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = CameraNode{};
    add_edge(parent.data(), id, _graph);
    _cameras.emplace_back(std::ref(_graph[id]));
    return _graph[id];
}

SceneNode& SceneGraph::addLight(std::string_view name, std::string_view parent) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = LightNode{};
    add_edge(parent.data(), id, _graph);
    _lights.emplace_back(std::ref(_graph[id]));
    return _graph[id];
}

SceneNode& SceneGraph::addEmpty(std::string_view name, std::string_view parent) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = EmptyNode{};
    add_edge(parent.data(), id, _graph);
    return _graph[id];
}

SceneNode& SceneGraph::addEmpty(std::string_view name) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = EmptyNode{};
    return _graph[id];
}

void SceneGraph::enable(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    _graph[v].node.enable();
}

void SceneGraph::disable(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    _graph[v].node.disable();
}

SceneNode& SceneGraph::get(std::string_view name) {
    auto v = *find_vertex(name.data(), _graph);
    return _graph[v];
}

}