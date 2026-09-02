#pragma once

#include "Camera.h"
#include "Light.h"
#include "Node.h"
#include "core/utils/Archive.h"

#if defined(_WIN32)
    // minwindef.h
    #ifdef near
        #undef near
    #endif
    #ifdef far
        #undef far
    #endif
#endif

namespace raum::scene {

template <class Archive>
void serialize(Archive& archive, PerspectiveFrustum& frustum) {
    archive(frustum.fov, frustum.aspect, frustum.near, frustum.far);
}

template <class Archive>
void serialize(Archive& archive, OrthoFrustum& frustum) {
    archive(frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.near, frustum.far);
}

struct CameraArchiveState {
    Projection projection{Projection::PERSPECTIVE};
    PerspectiveFrustum perspectiveFrustum;
    OrthoFrustum orthoFrustum;
    Vec3f position{0.0f};
    Quaternion orientation{};
    bool cullingEnabled{true};

};

struct NodeArchiveState {
    bool enabled{true};
    Mat4 transform{1.0f};

};

template <class Archive>
void serialize(Archive& archive, CameraArchiveState& state) {
    archive(state.projection,
            state.perspectiveFrustum,
            state.orthoFrustum,
            state.position,
            state.orientation,
            state.cullingEnabled);
}

template <class Archive>
void serialize(Archive& archive, NodeArchiveState& state) {
    archive(state.enabled, state.transform);
}

inline CameraArchiveState cameraArchiveState(const Camera& camera) {
    const auto& eye = camera.eye();
    return CameraArchiveState{
        .projection = eye.projectionType(),
        .perspectiveFrustum = eye.getPerspectiveFrustum(),
        .orthoFrustum = eye.getOrthoFrustum(),
        .position = eye.getPosition(),
        .orientation = eye.getOrientation(),
        .cullingEnabled = camera.cullingEnabled(),
    };
}

inline void applyCameraArchiveState(Camera& camera, const CameraArchiveState& state) {
    auto& eye = camera.eye();
    eye.setPosition(state.position);
    eye.setOrientation(state.orientation);
    eye.update();
    state.cullingEnabled ? camera.enableCulling() : camera.disableCulling();
    camera.update();
}

inline NodeArchiveState nodeArchiveState(const Node& node) {
    return NodeArchiveState{
        .enabled = node.enabled(),
        .transform = node.transform(),
    };
}

inline void applyNodeArchiveState(Node& node, const NodeArchiveState& state) {
    state.enabled ? node.enable() : node.disable();
    node.setTransform(state.transform);
}

template <class Archive>
void save(Archive& archive, const Node& node) {
    auto state = nodeArchiveState(node);
    archive(state);
}

template <class Archive>
void load(Archive& archive, Node& node) {
    NodeArchiveState state;
    archive(state);
    applyNodeArchiveState(node, state);
}

template <class Archive>
void save(Archive& archive, const Camera& camera) {
    auto state = cameraArchiveState(camera);
    archive(state);
}

template <class Archive>
void serialize(Archive& archive, AABB& bounds) {
    archive(bounds.minBound, bounds.maxBound);
}

template <class Archive>
void serialize(Archive& archive, Model& model) {
    // GPU-backed mesh renderers are rebuilt by the asset cache loader.
    archive(model.aabb());
}

template <class Archive>
void serialize(Archive&, Light&) {
    // Light currently has no persistent state.
}

} // namespace raum::scene

namespace cereal {

template <>
struct LoadAndConstruct<raum::scene::Camera> {
    template <class Archive>
    static void load_and_construct(Archive& archive, cereal::construct<raum::scene::Camera>& camera) {
        raum::scene::CameraArchiveState state;
        archive(state);

        if (state.projection == raum::scene::Projection::PERSPECTIVE) {
            camera(state.perspectiveFrustum);
        } else {
            camera(state.orthoFrustum);
        }
        raum::scene::applyCameraArchiveState(*camera.ptr(), state);
    }
};

} // namespace cereal
