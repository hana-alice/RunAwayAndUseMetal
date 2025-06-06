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

enum class ModelHint:uint32_t {
    NONE = 0,
    NO_CULLING = 1,
};
OPERABLE(ModelHint)

struct ModelNode {
    scene::ModelPtr model;
    ModelHint hint{ModelHint::NONE};
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
    using VertexType = SceneGraphImpl::vertex_descriptor;

    SceneGraph() = default;
    SceneGraph(const SceneGraph&) = delete;
    SceneGraph& operator=(const SceneGraph&) = delete;
    SceneGraph(SceneGraph&&) = delete;

    ModelNode& addModel(std::string_view name);
    CameraNode& addCamera(std::string_view name);
    LightNode& addLight(std::string_view name);
    ModelNode& addModel(std::string_view name, std::string_view parent);
    CameraNode& addCamera(std::string_view name, std::string_view parent);
    LightNode& addLight(std::string_view name, std::string_view parent);
    EmptyNode& addEmpty(std::string_view name, std::string_view parent);
    // root node
    EmptyNode& addEmpty(std::string_view name);

    SceneNode& get(std::string_view name);
    
    void enable(std::string_view name);
    void disable(std::string_view name);

    void reset() {};

    SceneGraphImpl& impl() { return _graph; }

    const SceneGraphImpl& impl() const { return _graph; }

    const std::vector<CameraNode*>& cameras() const {
        return _cameras;
    }

private:
    std::vector<CameraNode*> _cameras;

    SceneGraphImpl _graph;
};

using SceneGraphPtr = std::shared_ptr<SceneGraph>;

} // namespace raum::graph
