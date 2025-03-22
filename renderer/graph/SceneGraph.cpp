#include "SceneGraph.h"

namespace raum::graph {

ModelNode& SceneGraph::addModel(std::string_view name, std::string_view parent) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = ModelNode{};
    add_edge(parent.data(), id, _graph);
    auto& modelNode = std::get<ModelNode>(_graph[id].sceneNodeData);
    return modelNode;
}

ModelNode& SceneGraph::addModel(std::string_view name) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = ModelNode{};
    auto& modelNode = std::get<ModelNode>(_graph[id].sceneNodeData);
    return modelNode;
}

CameraNode& SceneGraph::addCamera(std::string_view name, std::string_view parent) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = CameraNode{};
    add_edge(parent.data(), id, _graph);
    auto& camNode = std::get<CameraNode>(_graph[id].sceneNodeData);
    return camNode;
}

CameraNode& SceneGraph::addCamera(std::string_view name) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = CameraNode{};
    auto& camNode = std::get<CameraNode>(_graph[id].sceneNodeData);
    return camNode;
}

LightNode& SceneGraph::addLight(std::string_view name, std::string_view parent) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = LightNode{};
    add_edge(parent.data(), id, _graph);
    auto& lightNode = std::get<LightNode>(_graph[id].sceneNodeData);
    return lightNode;
}

LightNode& SceneGraph::addLight(std::string_view name) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = LightNode{};
    auto& lightNode = std::get<LightNode>(_graph[id].sceneNodeData);
    return lightNode;
}

EmptyNode& SceneGraph::addEmpty(std::string_view name, std::string_view parent) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = EmptyNode{};
    add_edge(parent.data(), id, _graph);
    return std::get<EmptyNode>(_graph[id].sceneNodeData);
}

EmptyNode& SceneGraph::addEmpty(std::string_view name) {
    auto id = add_vertex(name.data(), _graph);
    _graph[id].sceneNodeData = EmptyNode{};
    return std::get<EmptyNode>(_graph[id].sceneNodeData);
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

} // namespace raum::graph