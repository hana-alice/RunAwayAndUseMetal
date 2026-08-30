#include "SceneGraph.h"
#include <stdexcept>

namespace raum::graph {

namespace {
SceneGraph::VertexType requireVertex(std::string_view name, SceneGraphImpl& graph) {
    const auto vertex = find_vertex(std::string{name}, graph);
    if (!vertex) {
        throw std::out_of_range("Scene node was not found: " + std::string{name});
    }
    return *vertex;
}
} // namespace

ModelNode& SceneGraph::addModel(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = ModelNode{};
    add_edge(requireVertex(parent, _graph), id, _graph);
    auto& modelNode = std::get<ModelNode>(_graph[id].sceneNodeData);
    return modelNode;
}

ModelNode& SceneGraph::addModel(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = ModelNode{};
    auto& modelNode = std::get<ModelNode>(_graph[id].sceneNodeData);
    return modelNode;
}

CameraNode& SceneGraph::addCamera(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = CameraNode{};
    add_edge(requireVertex(parent, _graph), id, _graph);
    auto& camNode = std::get<CameraNode>(_graph[id].sceneNodeData);
    return camNode;
}

CameraNode& SceneGraph::addCamera(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = CameraNode{};
    auto& camNode = std::get<CameraNode>(_graph[id].sceneNodeData);
    return camNode;
}

std::vector<const CameraNode*> SceneGraph::cameras() const {
    std::vector<const CameraNode*> result;
    result.reserve(boost::num_vertices(_graph));
    for (const auto vertex : boost::make_iterator_range(boost::vertices(_graph))) {
        if (std::holds_alternative<CameraNode>(_graph[vertex].sceneNodeData)) {
            result.emplace_back(&std::get<CameraNode>(_graph[vertex].sceneNodeData));
        }
    }
    return result;
}

LightNode& SceneGraph::addLight(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = LightNode{};
    add_edge(requireVertex(parent, _graph), id, _graph);
    auto& lightNode = std::get<LightNode>(_graph[id].sceneNodeData);
    return lightNode;
}

LightNode& SceneGraph::addLight(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = LightNode{};
    auto& lightNode = std::get<LightNode>(_graph[id].sceneNodeData);
    return lightNode;
}

EmptyNode& SceneGraph::addEmpty(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = EmptyNode{};
    add_edge(requireVertex(parent, _graph), id, _graph);
    return std::get<EmptyNode>(_graph[id].sceneNodeData);
}

EmptyNode& SceneGraph::addEmpty(std::string_view name) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = EmptyNode{};
    return std::get<EmptyNode>(_graph[id].sceneNodeData);
}

void SceneGraph::enable(std::string_view name) {
    auto v = requireVertex(name, _graph);
    _graph[v].node.enable();
}

void SceneGraph::disable(std::string_view name) {
    auto v = requireVertex(name, _graph);
    _graph[v].node.disable();
}

SceneNode& SceneGraph::get(std::string_view name) {
    auto v = requireVertex(name, _graph);
    return _graph[v];
}

} // namespace raum::graph
