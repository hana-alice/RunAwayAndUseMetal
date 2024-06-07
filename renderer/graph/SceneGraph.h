#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "Camera.h"
#include "GraphTypes.h"
#include "Light.h"
#include "Model.h"

namespace raum::graph {

struct ModelNode {
    scene::ModelPtr model;
};

struct CameraNode {
    scene::CameraPtr camera;
};

struct LightNode {
    scene::LightPtr light;
};

struct SceneNode {
    std::string name{};
    bool enable{true};
    std::variant<ModelNode, CameraNode, LightNode> sceneNodeData;
};

} // namespace raum::graph

namespace boost {
namespace graph {

template <>
struct internal_vertex_name<raum::graph::SceneNode> {
    typedef multi_index::member<raum::graph::SceneNode, std::string, &raum::graph::SceneNode::name> type;
};

template <>
struct internal_vertex_constructor<raum::graph::SceneNode> {
    typedef vertex_from_name<raum::graph::SceneNode> type;
};

} // namespace graph
} // namespace boost

namespace raum::graph {

using SceneGraphImpl = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, SceneNode, boost::no_property>;

class SceneGraph {
public:
    SceneGraph() = default;
    SceneGraph(const SceneGraph&) = delete;
    SceneGraph& operator=(const SceneGraph&) = delete;
    SceneGraph(SceneGraph&&) = delete;

    ModelNode& addModel(std::string_view name, std::string_view parent);
    CameraNode& addCamera(std::string_view name, std::string_view parent, const scene::Frustum& frustum, scene::Projection proj);
    LightNode& addLight(std::string_view name, std::string_view parent);
    
    void enable(std::string_view name);
    void disable(std::string_view name);

    SceneNode& sceneRoot() const;

    const SceneGraphImpl& impl() const { return _graph; }

private:
    SceneGraphImpl _graph;
};

} // namespace raum::graph
