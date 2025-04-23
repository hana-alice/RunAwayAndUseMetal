#include "Camera.h"

namespace raum::scene {

Camera::Camera(const raum::scene::PerspectiveFrustum& frustum) : _eye(frustum) {
}

Camera::Camera(const OrthoFrustum& frustum) : _eye(frustum) {
}

void Camera::update() {
    bool dirty = _eye.update();
    if (dirty) {
        if (_eye.projectionType() == Projection::PERSPECTIVE) {
            const auto& camRight = glm::cross(_eye.up(), _eye.forward());
            const auto& camUp = _eye.up();
            const auto& frustum = _eye.getPerspectiveFrustum();

            auto rad = utils::toRadian(frustum.fov);
            auto halfFarPlaneHeight = frustum.far * tan(rad.value * 0.5f);
            auto halfFarPlaneWidth = frustum.aspect * halfFarPlaneHeight;
            auto zFarVec = frustum.far * _eye.forward();
            // near plane
            _frustumPlanes[0].point = _eye.getPosition() + frustum.near * _eye.forward();
            _frustumPlanes[0].normal = _eye.forward();

            // far plane
            _frustumPlanes[1].point = _eye.getPosition() + zFarVec;
            _frustumPlanes[1].normal = -_eye.forward();

            // left plane
            _frustumPlanes[2].point = _eye.getPosition();
            _frustumPlanes[2].normal = glm::cross(camUp, zFarVec - camRight * halfFarPlaneWidth);
            _frustumPlanes[2].normal = glm::normalize(_frustumPlanes[2].normal);

            // right plane
            _frustumPlanes[3].point = _eye.getPosition();
            _frustumPlanes[3].normal = glm::cross(zFarVec + camRight * halfFarPlaneWidth, camUp);
            _frustumPlanes[3].normal = glm::normalize(_frustumPlanes[3].normal);

            // top plane
            _frustumPlanes[4].point = _eye.getPosition();
            _frustumPlanes[4].normal = glm::cross(camRight, zFarVec + camUp * halfFarPlaneHeight);
            _frustumPlanes[4].normal = glm::normalize(_frustumPlanes[4].normal);

            // bottom plane
            _frustumPlanes[5].point = _eye.getPosition();
            _frustumPlanes[5].normal = glm::cross(zFarVec - camUp * halfFarPlaneHeight, camRight);
            _frustumPlanes[5].normal = glm::normalize(_frustumPlanes[5].normal);

        } else if (_eye.projectionType() == Projection::ORTHOGRAPHIC) {
            const auto& frustum = _eye.getOrthoFrustum();
            const auto& camRight = glm::cross(_eye.up(), _eye.forward());
            const auto& camUp = _eye.up();
            // near plane
            _frustumPlanes[0].point = _eye.getPosition() + frustum.near  * _eye.forward();
            _frustumPlanes[0].normal = _eye.forward();

            // far plane
            _frustumPlanes[1].point = _eye.getPosition() + frustum.far * _eye.forward();
            _frustumPlanes[1].normal = -_eye.forward();

            // left plane
            _frustumPlanes[2].point = _eye.getPosition() - frustum.left * camRight;
            _frustumPlanes[2].normal = camRight;

            // right plane
            _frustumPlanes[3].point = _eye.getPosition() + frustum.right * camRight;
            _frustumPlanes[3].normal = -camRight;

            // top plane
            _frustumPlanes[4].point = _eye.getPosition() + frustum.top * camUp;
            _frustumPlanes[4].normal = -camUp;

            // bottom plane
            _frustumPlanes[5].point = _eye.getPosition() - frustum.bottom * camRight;
            _frustumPlanes[5].normal = camRight;

        } else {
            raum_error("unsupported projection type: {}", static_cast<uint8_t>(_eye.projectionType()));
        }
    }
}

} // namespace raum::scene