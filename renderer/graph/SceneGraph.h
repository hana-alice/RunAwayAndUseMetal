#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "Camera.h"
#include "GraphTypes.h"
#include "Light.h"
#include "Model.h"
#include "Node.h"

namespace raum::graph {

struct EmptyNode {
};

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
    scene::Node node;
    std::variant<ModelNode, CameraNode, LightNode, EmptyNode> sceneNodeData;
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

    SceneNode& addModel(std::string_view name, std::string_view parent);
    SceneNode& addCamera(std::string_view name, std::string_view parent);
    SceneNode& addLight(std::string_view name, std::string_view parent);
    SceneNode& addEmpty(std::string_view name, std::string_view parent);
    // root node
    SceneNode& addEmpty(std::string_view name);

    SceneNode& get(std::string_view name);
    
    void enable(std::string_view name);
    void disable(std::string_view name);

    SceneNode& sceneRoot() const;

    void reset();

    const SceneGraphImpl& impl() const { return _graph; }

    auto& models() {
        return _models;
    }

    auto& cameras() {
        return _cameras;
    }

    auto& lights() {
        return _lights;
    }


private:
    SceneGraphImpl _graph;
    std::vector<std::reference_wrapper<SceneNode>> _models;
    std::vector<std::reference_wrapper<SceneNode>> _cameras;
    std::vector<std::reference_wrapper<SceneNode>> _lights;

};

} // namespace raum::graph
