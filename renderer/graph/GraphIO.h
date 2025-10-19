#pragma once
#include "SceneIO.h"

namespace raum{
namespace graph {
template <class Archive>
void serialize(Archive& ar, CameraNode& cameraNode) {
    ar(cameraNode.camera);
}

template <class Archive>
void serialize(Archive& ar, ModelNode& modelNode) {
    ar(modelNode.model);
    ar(modelNode.hint);
}

template <class Archive>
void serialize(Archive& ar, LightNode& lightNode) {
    ar(lightNode.light);
}

template <class Archive>
void serialize(Archive& ar, EmptyNode& emptyNode) {
    // ar(emptyNode);
}

template <class Archive>
void serialize(Archive& ar, SceneNode& node) {
    ar(node.name);
    ar(node.node);
    ar(node.sceneNodeData);
}

} // namespace raum::graph


template<>
inline void utils::InputArchive::read(graph::SceneGraph& sg) {
    auto& ar = *iarchive;
    sg.reset();

    auto vnum{0};
    ar >> vnum;

    for (int i = 0; i < vnum; i++) {
        graph::SceneNode node;
        ar >> node.name;
        ar >> node.node;
        ar >> node.sceneNodeData;

        std::visit(overloaded{
                       [&](graph::ModelNode&) {
                           sg.addModel(node.name);
                           auto& data = sg.get(node.name);
                           data.node = node.node;
                           data.sceneNodeData = node.sceneNodeData;
                       },
                       [&](graph::CameraNode&) {
                           sg.addCamera(node.name);
                           auto& data = sg.get(node.name);
                           data.node = node.node;
                           data.sceneNodeData = node.sceneNodeData;
                       },
                       [&](graph::LightNode&) {
                           sg.addLight(node.name);
                           auto& data = sg.get(node.name);
                           data.node = node.node;
                           data.sceneNodeData = node.sceneNodeData;
                       },
                       [&](graph::EmptyNode&) {
                           sg.addEmpty(node.name);
                           auto& data = sg.get(node.name);
                           data.node = node.node;
                           data.sceneNodeData = node.sceneNodeData;
                       },
                       [&](auto&& arg) {},
                   },
                   node.sceneNodeData);
    }

    auto eNum{0};
    ar >> eNum;
    auto& impl = sg.impl();
    for (int i = 0; i < eNum; i++) {
        graph::SceneGraph::VertexType v1, v2;
        ar(v1, v2);
        add_edge(v1, v2, impl);
    }
}

template<>
inline void utils::OutputArchive::write(const graph::SceneGraph& sg) {
    const auto& graph = sg.impl();
    auto& ar = *oarchive;
    ar << boost::num_vertices(graph);
    for (auto [it, end] = vertices(graph); it != end; ++it) {
        auto v = *it;
        auto& node = graph[v];
        ar << node.name;
        ar << node.node;
        ar << node.sceneNodeData;
    }
    ar << boost::num_edges(graph);
    for (auto [it, end] = edges(graph); it != end; ++it) {
        auto e = *it;
        ar(e.m_source, e.m_target);
    }
}

}  // namespace raum