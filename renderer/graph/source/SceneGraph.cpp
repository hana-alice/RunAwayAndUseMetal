#include "SceneGraph.h"

using boost::add_vertex;
using boost::get;
using boost::vertices;

namespace raum::graph {


ModelNode SceneGraph::addModel(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = scene::Model();
    auto& model = std::get<scene::Model>(_graph[id].sceneNodeData);
    return ModelNode{model};
}

CameraNode SceneGraph::addCamera(std::string_view name, std::string_view parent, const scene::Frustum& frustum, scene::Projection proj) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = scene::Camera(frustum, proj);
    auto camera = std::get<scene::Camera>(_graph[id].sceneNodeData);
    return CameraNode{camera};
}

LightNode SceneGraph::addLight(std::string_view name, std::string_view parent) {
    auto id = add_vertex(std::string{name}, _graph);
    _graph[id].sceneNodeData = scene::Light();
    auto light = std::get<scene::Light>(_graph[id].sceneNodeData);
    return LightNode{light};
}

}