#include "Eye.h"

namespace raum::scene {
Eye::Eye(const PerspectiveFrustum& frustum)
    : _perspectiveFrustum(frustum), _projectionType(Projection::PERSPECTIVE) {
    _projectionMat = glm::perspective(glm::radians(frustum.fov.value), frustum.aspect, frustum.near, frustum.far);
}

Eye::Eye(const OrthoFrustum& frustum)
    : _orthoFrustum(frustum), _projectionType(Projection::ORTHOGRAPHIC) {
    _projectionMat = glm::ortho(frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.near, frustum.far);
}

void Eye::setPosition(const Vec3f& pos) {
    _position = pos;
    _dirty = true;
}

void Eye::setPosition(float x, float y, float z) {
    _position = {x, y, z};
    _dirty = true;
}

void Eye::setOrientation(const raum::Quaternion& quat) {
    _orientation = quat;
    _up = quat * InitialUp;
    _forward = quat * InitialForward;
    _dirty = true;
}

void Eye::rotate(const Vec3f& axis, utils::Degree degree) {
    utils::Radian rad{glm::radians(degree.value)};
    rotate(axis, rad);
}

void Eye::rotate(const Vec3f& axis, utils::Radian radian) {
    auto q = glm::angleAxis(radian.value, axis);
    _orientation = q * _orientation;
    _forward = _orientation * InitialForward;
    _up = _orientation * InitialUp;
    _dirty = true;
}

void Eye::translate(const Vec3f& delta) {
    _position += delta;
    _dirty = true;
}

void Eye::translate(float x, float y, float z) {
    _position.x += x;
    _position.y += y;
    _position.z += z;
    _dirty = true;
}

void Eye::lookAt(const Vec3f& pos, const Vec3f& up) {
    auto dir = glm::normalize(pos - _position);
    auto upN = glm::normalize(up);
    _orientation = glm::quatLookAt(dir, upN);
    _forward = dir;
    auto right = glm::cross(upN, _forward);
    _up = glm::cross(_forward, right);
    _dirty = true;
}

const PerspectiveFrustum& Eye::getPerspectiveFrustum() const {
    return _perspectiveFrustum;
}

const OrthoFrustum& Eye::getOrthoFrustum() const {
    return _orthoFrustum;
}

const Vec3f& Eye::getPosition() const {
    return _position;
}

const Quaternion& Eye::getOrientation() const {
    return _orientation;
}

const Mat4& Eye::attitude() const {
    return _attitude;
}

const Mat4& Eye::projection() const {
    return _projectionMat;
}

const Vec3f Eye::forward() const {
    return _forward;
}

const Vec3f Eye::up() const {
    return _up;
}

void Eye::setTransform(const Mat4& mat) {
    _attitude = mat;
    _dirty = true;
}

Projection Eye::projectionType() const {
    return _projectionType;
}

bool Eye::update() {
    bool dirty = _dirty;
    if (dirty) {
        Mat4 rot(conjugate(_orientation));
        Mat4 trans(glm::translate(Mat4(1.0f), -_position));
        _attitude = rot * trans;
        _dirty = false;
    }
    return dirty;
}

Eye::~Eye() {
}
} // namespace raum::scene