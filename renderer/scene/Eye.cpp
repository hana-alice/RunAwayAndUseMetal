#include "Eye.h"

namespace raum::scene {
Eye::Eye(const PerspectiveFrustum& frustum)
    : _perspectiveFrustum(frustum), _projection(Projection::PERSPECTIVE) {
    _projectionMat = glm::perspective(glm::radians(frustum.fov), frustum.aspect, frustum.near, frustum.far);
}

Eye::Eye(const OrthoFrustum& frustum)
    : _orthoFrustum(frustum), _projection(Projection::ORTHOGRAPHIC) {
    _projectionMat = glm::ortho(frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.near, frustum.far);
}

void Eye::setPosition(const Vec3f& pos) {
    _position = pos;
}

void Eye::setPosition(float x, float y, float z) {
    _position = {x, y, z};
}

void Eye::setOrientation(const raum::Quaternion& quat) {
    _orientation = quat;
    _up = quat * _up;
    _forward = quat * _forward;
}

void Eye::rotate(const Vec3f& axis, Degree degree) {
    Radian rad{glm::radians(degree.value)};
    rotate(axis, rad);
}

void Eye::rotate(const Vec3f& axis, Radian radian) {
    auto q = glm::angleAxis(radian.value, axis);
    _orientation = q * _orientation;
    _forward = q * forward();
    _up = q * _up;
}

void Eye::translate(const Vec3f& delta) {
    _position += delta;
}

void Eye::translate(float x, float y, float z) {
    _position.x += x;
    _position.y += y;
    _position.z += z;
}

void Eye::lookAt(const Vec3f& pos, const Vec3f& up) {
    auto dir = glm::normalize(pos - _position);
    auto upN = glm::normalize(up);
    _orientation = glm::quatLookAt(dir, upN);
    _forward = dir;
    auto right = glm::cross(_forward, upN);
    _up = glm::cross(right, _forward);
}

void Eye::setFrustum(const PerspectiveFrustum& frustum) {
    _perspectiveFrustum = frustum;
    _projectionMat = glm::perspective(glm::radians(frustum.fov), frustum.aspect, frustum.near, frustum.far);
}

void Eye::setFrustum(const OrthoFrustum& frustum) {
    _orthoFrustum = frustum;
    _projectionMat = glm::ortho(frustum.left, frustum.right, frustum.bottom, frustum.top, frustum.near, frustum.far);
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

const Mat4& Eye::inverseAttitude() const {
    return _attitudeInversed;
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
    _attitudeInversed = glm::inverse(_attitude);
}

void Eye::update() {
    Mat4 rot(_orientation);
    Mat4 trans(glm::translate(Mat4(1.0f), _position));
    _attitude = trans * rot;
    _attitudeInversed = glm::inverse(_attitude);
}

Eye::~Eye() {
}
} // namespace raum::scene