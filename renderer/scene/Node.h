#pragma once
#include "common.h"
#include <string>
namespace raum::scene {

class Node {
public:
    Node() = default;

    void enable();
    void disable();
    void setTransform(const Mat4& transform);
    void setOrientation(const Quaternion& quat);
    void setTranslation(const Vec3f& trans);
    void setScale(const Vec3f& scale);
    void update();

    const Mat4& transform() const;
    bool enabled() const;

private:
    bool _enable{true};
    bool _dirty{false};

    Mat4 _transform{1.0f};
    Vec3f _translation;
    Quaternion _orientation;
    Vec3f _scale{1.0f};

};

}