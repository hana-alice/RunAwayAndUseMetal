#pragma once
#include "common.h"
namespace raum::scene {
class Eye {
public:
    explicit Eye(const Frustum& frustum, Projection projection);
    ~Eye();

    void setPosition(const Vec3f& pos);

    void setPosition(float x, float y, float z);

    void setRotation(const Quaternion& quat);

    void rotate(const Vec3f& axis, Degree degree);

    void rotate(const Vec3f& axis, Radian radian);

    void translate(const Vec3f& delta);

    void translate(float x, float y, float z);

    void lookAt(const Vec3f& pos, const Vec3f& up);

    const Frustum& getFrustum() const;

    const Vec3f& getPosition() const;

    const Quaternion& getRotation() const;

    const Mat4& attitude() const;

    const Mat4& projection() const;

private:
    Projection _projection{Projection::PERSPECTIVE};
    Frustum _frustum{};
    Vec3f _position{0.0f};
    Quaternion _rotation{};
    Mat4 _attitude{1.0f};
    Mat4 _projectionMat{1.0f};
};

} // namespace raum::scene