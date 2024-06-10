#include "Node.h"

namespace raum::scene {

void Node::enable() {
    _enable = true;
}

void Node::disable() {
    _enable = false;
}

void Node::setTransform(const raum::Mat4 &transform) {
    _transform = transform;
}

void Node::setTranslation(const Vec3f &trans) {
    _translation = trans;
    _dirty = true;
}

void Node::setScale(const Vec3f &scale) {
    _scale = scale;
    _dirty = true;
}

void Node::setOrientation(const Quaternion &quat) {
    _orientation = quat;
    _dirty = true;
}

void Node::update() {
    if(_dirty) {
        Mat4 rot = Mat4(_orientation);
        auto scale = glm::scale(Mat4(1.0f), _scale);
        auto trans = glm::translate(Mat4(1.0f), _translation);
        _transform = scale * trans * rot;
    }
}

bool Node::enabled() const {
    return _enable;
}

const Mat4 &Node::transform() const {
    return _transform;
}



}