#pragma once
#include <stdint.h>
#include <vector>
#include "RHIDefine.h"
#include "Model.h"
#include <span>
#include "Node.h"
#include "Camera.h"
#include "core/utils/Archive.h"

namespace raum::scene {

class Scene {
public:
    Scene() {};

    std::vector<Model> models;
};

struct BVHNode {
    BVHNode* left{nullptr};
    BVHNode* right{nullptr};
    AABB aabb{};
    std::span<RenderablePtr> children;
};

void buildBVH(BVHNode* node, std::vector<scene::ModelPtr>& models);

// should be rendered / inside or intersect with frustum
bool frustumCulling(const scene::FrustumPlanes& frustum, const AABB& aabb);

float distance(const Vec3f& point, const Plane& plane);

} // namespace raum::scene


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
    ar(frustum.fov.value);
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


} // namespace cereal