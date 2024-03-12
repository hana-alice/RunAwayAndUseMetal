#include "Eye.h"

namespace raum::scene {
Eye::Eye(const Frustum& frustum, Projection projection)
: _frustum(frustum), _projection(projection) {
    _projectionMat = glm::perspective(glm::radians(frustum.fov), frustum.aspect, frustum.near, frustum.far);
}

void Eye::setPosition(const Vec3f& pos) {
    _position = pos;
    _attitude = glm::translate(Mat4(), pos);
}

void Eye::setPosition(float x, float y, float z) {
    _position = {x, y, z};
    _attitude = glm::translate(Mat4(), _position);
}

void Eye::setRotation(const Quaternion& quat) {
    _rotation = quat;
    _attitude = _attitude * (Mat4)_rotation;
}

void Eye::rotate(const Vec3f& axis, Degree degree) {
    Radian rad{glm::radians(degree.value)};
    _rotation = glm::rotate(_rotation, rad.value, axis);
    _attitude = _attitude * (Mat4)_rotation;
}

void Eye::rotate(const Vec3f& axis, Radian radian) {
    _rotation = glm::rotate(_rotation, radian.value, axis);
    _attitude = _attitude * (Mat4)_rotation;
}

void Eye::translate(const Vec3f& delta) {
    _position += delta;
    _attitude = glm::translate(_attitude, delta);
}

void Eye::translate(float x, float y, float z) {
    Vec3f delta{x, y, z};
    _position += delta;
    _attitude = glm::translate(_attitude, delta);
}

void Eye::lookAt(const Vec3f& pos, const Vec3f& up) {
    _attitude = glm::lookAt(_position, pos, up);
    Mat3 rotation = Mat3(_attitude);
    _rotation = Mat3(rotation);
}

const Frustum& Eye::getFrustum() const {
    return _frustum;
}

const Vec3f& Eye::getPosition() const {
    return _position;
}

const Quaternion& Eye::getRotation() const {
    return _rotation;
}

const Mat4& Eye::attitude() const {
    return _attitude;
}

const Mat4& Eye::projection() const {
    return _projectionMat;
}

Eye::~Eye() {
}

} // namespace raum::scene