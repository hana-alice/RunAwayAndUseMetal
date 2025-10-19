#pragma once
#include "Node.h"
#include "Camera.h"
#include "core/utils/Archive.h"
#include "Light.h"

#if defined(_WIN32)
    // minwindef.h
    #ifdef near
        #undef near
    #endif
    #ifdef far
        #undef far
    #endif
#endif

namespace cereal {

template <>
struct LoadAndConstruct<raum::scene::Camera> {
    template <class Archive>
    static void load_and_construct(Archive& ar, cereal::construct<raum::scene::Camera>& camera) {
        raum::scene::Projection projection;
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
}

namespace raum::scene {

template <class Archive>
void serialize(Archive& ar, Node& node) {
    bool enable{false};
    ar(enable);

    Mat4 transform;
    ar(transform);

    enable ? node.enable() : node.disable();
    node.setTransform(transform);
}

template <class Archive>
void serialize(Archive& ar, utils::Degree& degree) {
    ar(degree.value);
}

template <class Archive>
void serialize(Archive& ar, PerspectiveFrustum& frustum) {
    ar(frustum.fov);
    ar(frustum.aspect);
    ar(frustum.near);
    ar(frustum.far);
}

template <class Archive>
void serialize(Archive& ar, OrthoFrustum& frustum) {
    ar(frustum.left);
    ar(frustum.right);
    ar(frustum.bottom);
    ar(frustum.top);
    ar(frustum.near);
    ar(frustum.far);
}

template <class Archive>
void save(Archive& ar, const Camera& camera) {
    const auto& eye = camera.eye();
    ar(eye.projection());
    ar(eye.getPerspectiveFrustum());
    ar(eye.getOrthoFrustum());
    ar(eye.getPosition());
    ar(eye.up());
    ar(eye.getOrientation());
}

//
// template <class Archive>
// void save(Archive& ar, const CameraPtr& camera) {
//     save(ar, *camera);
// }
//
// template <class Archive>
// void save(Archive& ar, CameraPtr& camera) {
//     save(ar, *camera);
// }
//
// template <class Archive>
// void save(Archive& ar, CameraPtr camera) {
//     save(ar, *camera);
// }
//
// template <class Archive>
// void load(Archive& ar, CameraPtr& camera) {
//     Projection projection;
//
//     ar(projection);
//     PerspectiveFrustum perspectiveFrustum;
//     OrthoFrustum orthoFrustum;
//     ar(perspectiveFrustum);
//     ar(orthoFrustum);
//
//     if (projection == Projection::PERSPECTIVE) {
//         camera = std::make_shared<Camera>(perspectiveFrustum);
//     } else {
//         camera = std::make_shared<Camera>(orthoFrustum);
//     }
//     auto& eye = camera->eye();
//     Vec3f eyePosition;
//     Vec3f eyeUp;
//     Quaternion eyeOrientation;
//     ar(eyePosition, eyeUp, eyeOrientation);
//     eye.setPosition(eyePosition);
//     eye.setOrientation(eyeOrientation);
//     eye.update();
// }

template <class Archive>
void serialize(Archive& ar, raum::scene::AABB& aabb) {
    ar(aabb.minBound, aabb.maxBound);
}

template <class Archive>
void serialize(Archive& ar, ModelPtr& model) {
    ar(*model);
}

template <class Archive>
void serialize(Archive& ar, ModelPtr model) {
    ar(*model);
}

template <class Archive>
void serialize(Archive& ar, Model& model) {
    // serialized when loading gltf
    // ar(model.meshRenderers());
    ar(model.aabb());
}

template <class Archive>
void serialize(Archive& ar, LightPtr& light) {
    ar(*light);
}

template <class Archive>
void serialize(Archive& ar, LightPtr light) {
    ar(*light);
}

template <class Archive>
void serialize(Archive& ar, Light& light) {
    raum_error("not implemented");
    // ar(light.color());
}


} // namespace scene