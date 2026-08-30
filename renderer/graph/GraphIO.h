#pragma once

#include <cstdint>
#include <stdexcept>
#include "SceneGraph.h"
#include "SceneIO.h"

namespace raum::graph {

template <class Archive>
void serialize(Archive& archive, CameraNode& node) {
    archive(node.camera);
}

template <class Archive>
void serialize(Archive& archive, ModelNode& node) {
    archive(node.model, node.hint);
}

template <class Archive>
void serialize(Archive& archive, LightNode& node) {
    archive(node.light);
}

template <class Archive>
void serialize(Archive&, EmptyNode&) {
}

template <class Archive>
void serialize(Archive& archive, SceneNode& node) {
    archive(node.name, node.node, node.sceneNodeData);
}

struct SceneGraphEdgeArchive {
    uint64_t source{0};
    uint64_t target{0};

    template <class Archive>
    void serialize(Archive& archive) {
        archive(source, target);
    }
};

struct SceneGraphArchive {
    std::vector<SceneNode> nodes;
    std::vector<SceneGraphEdgeArchive> edges;

    template <class Archive>
    void serialize(Archive& archive) {
        archive(nodes, edges);
    }
};

} // namespace raum::graph

namespace raum {

template <>
inline void utils::InputArchive::read(graph::SceneGraph& sceneGraph) {
    graph::SceneGraphArchive savedGraph;
    (*iarchive)(savedGraph);

    sceneGraph.reset();
    for (auto& savedNode : savedGraph.nodes) {
        std::visit(overloaded{
                       [&](graph::ModelNode&) { sceneGraph.addModel(savedNode.name); },
                       [&](graph::CameraNode&) { sceneGraph.addCamera(savedNode.name); },
                       [&](graph::LightNode&) { sceneGraph.addLight(savedNode.name); },
                       [&](graph::EmptyNode&) { sceneGraph.addEmpty(savedNode.name); },
                   },
                   savedNode.sceneNodeData);

        auto& node = sceneGraph.get(savedNode.name);
        node.node = std::move(savedNode.node);
        node.sceneNodeData = std::move(savedNode.sceneNodeData);
    }

    const auto vertexCount = boost::num_vertices(sceneGraph.impl());
    for (const auto& edge : savedGraph.edges) {
        if (edge.source >= vertexCount || edge.target >= vertexCount) {
            throw std::runtime_error("Serialized scene graph contains an invalid edge");
        }
        add_edge(static_cast<graph::SceneGraph::VertexType>(edge.source),
                 static_cast<graph::SceneGraph::VertexType>(edge.target),
                 sceneGraph.impl());
    }
}

template <>
inline void utils::OutputArchive::write(const graph::SceneGraph& sceneGraph) {
    const auto& impl = sceneGraph.impl();
    graph::SceneGraphArchive savedGraph;
    savedGraph.nodes.reserve(boost::num_vertices(impl));
    savedGraph.edges.reserve(boost::num_edges(impl));

    for (auto [it, end] = vertices(impl); it != end; ++it) {
        savedGraph.nodes.emplace_back(impl[*it]);
    }
    for (auto [it, end] = edges(impl); it != end; ++it) {
        savedGraph.edges.emplace_back(graph::SceneGraphEdgeArchive{
            .source = static_cast<uint64_t>(source(*it, impl)),
            .target = static_cast<uint64_t>(target(*it, impl)),
        });
    }

    (*oarchive)(savedGraph);
}

} // namespace raum
