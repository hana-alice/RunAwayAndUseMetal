#pragma once
#include <boost/graph/adjacency_list.hpp>
#include <variant>
#include "GraphTypes.h"
#include "Model.h"
#include "Light.h"
#include "Camera.h"

namespace raum::graph {

struct SceneNode {
    std::string name{};
    std::variant<scene::Model, scene::Light, scene::Camera> sceneNodeData;
};

}

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



namespace raum::graph{

using SceneGraphImpl = boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS, SceneNode, boost::no_property>;

class SceneGraph {
public:
    SceneGraph() = default;
    SceneGraph(const SceneGraph&) = delete;
    SceneGraph& operator=(const SceneGraph&) = delete;
    SceneGraph(SceneGraph&&) = delete;

    scene::Model& addModel(std::string_view name, std::string_view parent);
    scene::Camera& addCamera(std::string_view name, std::string_view parent);
    scene::Light& addLight(std::string_view name, std::string_view parent);

    SceneNode& sceneRoot() const;

private:
    SceneGraphImpl _graph;

};


}
