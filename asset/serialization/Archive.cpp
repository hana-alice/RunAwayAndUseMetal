#include "Archive.h"

#include <tiny_gltf.h>

#include <fstream>

#include "GraphTypes.h"
#include "cereal/access.hpp"
#include "cereal/archives/binary.hpp"
#include "core/utils/log.h"

namespace cereal {

template <class Archive>
void serialize(Archive& ar, raum::scene::Node& node) {
    bool enable{false};
    ar(enable);

    raum::Mat4 transform;
    ar(transform);

    enable ? node.enable() : node.disable();
    node.setTransform(transform);
}

template <class Archive>
void serialize(Archive& ar, raum::scene::PerspectiveFrustum& frustum) {
    ar(frustum.fov);
    ar(frustum.aspect);
    ar(frustum.near);
    ar(frustum.far);
}

template <class Archive>
void serialize(Archive& ar, raum::scene::OrthoFrustum& frustum) {
    ar(frustum.left);
    ar(frustum.right);
    ar(frustum.bottom);
    ar(frustum.top);
    ar(frustum.near);
    ar(frustum.far);
}


template <>
struct LoadAndConstruct<raum::scene::Camera> {
    template <class Archive>
    static void load_and_construct(Archive& ar, cereal::construct<raum::scene::Camera>& camera) {
        raum::scene::Projection projection;

        ar(projection);
        raum::scene::PerspectiveFrustum perspectiveFrustum;
        raum::scene::OrthoFrustum orthoFrustum;
        ar(perspectiveFrustum);
        ar(orthoFrustum);

        if (projection == raum::scene::Projection::PERSPECTIVE) {
            camera(perspectiveFrustum);
        } else {
            camera(orthoFrustum);
        }
        auto& eye = camera->eye();
        raum::Vec3f eyePosition;
        raum::Vec3f eyeUp;
        raum::Quaternion eyeOrientation;
        ar(eyePosition, eyeUp, eyeOrientation);
        eye.setPosition(eyePosition);
        eye.setOrientation(eyeOrientation);
        eye.update();
    }
};


template <class Archive>
void serialize(Archive& ar, raum::graph::CameraNode& cameraNode) {
    ar(cameraNode.camera);
}

template <class Archive>
void save(Archive& ar, const raum::scene::Camera& camera) {
    const auto& eye = camera.eye();
    ar(
        eye.projection(),
        eye.getPerspectiveFrustum(),
        eye.getOrthoFrustum(),
        eye.getPosition(),
        eye.up(),
        eye.getOrientation());
}

//
// template <class Archive>
// void save(Archive& ar, const raum::scene::CameraPtr& camera) {
//     save(ar, *camera);
// }
//
// template <class Archive>
// void save(Archive& ar, raum::scene::CameraPtr& camera) {
//     save(ar, *camera);
// }
//
// template <class Archive>
// void save(Archive& ar, raum::scene::CameraPtr camera) {
//     save(ar, *camera);
// }
//
// template <class Archive>
// void load(Archive& ar, raum::scene::CameraPtr& camera) {
//     raum::scene::Projection projection;
//
//     ar(projection);
//     raum::scene::PerspectiveFrustum perspectiveFrustum;
//     raum::scene::OrthoFrustum orthoFrustum;
//     ar(perspectiveFrustum);
//     ar(orthoFrustum);
//
//     if (projection == raum::scene::Projection::PERSPECTIVE) {
//         camera = std::make_shared<raum::scene::Camera>(perspectiveFrustum);
//     } else {
//         camera = std::make_shared<raum::scene::Camera>(orthoFrustum);
//     }
//     auto& eye = camera->eye();
//     raum::Vec3f eyePosition;
//     raum::Vec3f eyeUp;
//     raum::Quaternion eyeOrientation;
//     ar(eyePosition, eyeUp, eyeOrientation);
//     eye.setPosition(eyePosition);
//     eye.setOrientation(eyeOrientation);
//     eye.update();
// }

template <class Archive>
void serialize(Archive& ar, raum::graph::ModelNode& modelNode) {
    ar(modelNode.model);
}

template <class Archive>
void serialize(Archive& ar, raum::scene::ModelPtr& model) {
    ar(*model);
}

template <class Archive>
void serialize(Archive& ar, raum::scene::ModelPtr model) {
    ar(*model);
}

template <class Archive>
void serialize(Archive& ar, raum::scene::Model& model) {
    // serialized when loading gltf
    // ar(model.meshRenderers());
    ar(model.aabb());
}

template <class Archive>
void serialize(Archive& ar, raum::scene::AABB& aabb) {
    ar(aabb.maxBound, aabb.minBound);
}

template <class Archive>
void serialize(Archive& ar, raum::graph::LightNode& lightNode) {
    ar(lightNode.light);
}

template <class Archive>
void serialize(Archive& ar, raum::scene::LightPtr& light) {
    ar(*light);
}

template <class Archive>
void serialize(Archive& ar, raum::scene::LightPtr light) {
    ar(*light);
}

template <class Archive>
void serialize(Archive& ar, raum::scene::Light& light) {
    raum::raum_error("not implemented");
    // ar(light.color());
}

template <class Archive>
void serialize(Archive& ar, raum::graph::EmptyNode& emptyNode) {
    // ar(emptyNode);
}

template <class Archive>
void serialize(Archive& ar, raum::graph::SceneNode& node) {
    ar(node.name);
    ar(node.node);
    ar(node.sceneNodeData);
}
} // namespace cereal

namespace raum::asset::serialize {

using boost::edges;
using boost::vertices;

InputArchive::InputArchive(const std::filesystem::path& filePath) {
    is = std::ifstream(filePath.string(), std::ios::binary);
    if (!is) {
        raum_error("Could not find file: %s", filePath.string());
    }
    iarchive = std::make_shared<cereal::BinaryInputArchive>(is);
}

void InputArchive::read(graph::SceneGraph& sg) {
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

OutputArchive::OutputArchive(const std::filesystem::path& filePath) {
    os = std::ofstream(filePath.string(), std::ios::binary | std::ios::trunc);
    if (!os) {
        raum_error("Failed to open file: %s", filePath.string());
    }
    oarchive = std::make_shared<cereal::BinaryOutputArchive>(os);
}

void OutputArchive::write(const graph::SceneGraph& sg) {
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

void OutputArchive::write(const uint8_t* data, uint32_t size) {
    auto& ar = *oarchive;
    ar << cereal::binary_data(data, size);
}


} // namespace raum::asset::serialize