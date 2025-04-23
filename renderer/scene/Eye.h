#pragma once
#include "common.h"
namespace raum::scene {
class Eye {
public:
    explicit Eye(const PerspectiveFrustum& frustum);
    explicit Eye(const OrthoFrustum& frustum);
    ~Eye();

    void setPosition(const Vec3f& pos);

    void setPosition(float x, float y, float z);

    void setOrientation(const Quaternion& quat);

    void setTransform(const Mat4& mat);

    void rotate(const Vec3f& axis, utils::Degree degree);

    void rotate(const Vec3f& axis, utils::Radian radian);

    void translate(const Vec3f& delta);

    void translate(float x, float y, float z);

    void lookAt(const Vec3f& pos, const Vec3f& up);

    const PerspectiveFrustum& getPerspectiveFrustum() const;

    const OrthoFrustum& getOrthoFrustum() const;

    const Vec3f& getPosition() const;

    const Quaternion& getOrientation() const;

    const Mat4& attitude() const;

    const Mat4& projection() const;

    const Vec3f forward() const;

    const Vec3f up() const;

    bool update();

    Projection projectionType() const;

    static constexpr Vec3f InitialUp{0.0f, 1.0f, 0.0f};
    static constexpr Vec3f InitialForward{0.0f, 0.0f, 1.0f};

private:
    Projection _projectionType{Projection::PERSPECTIVE};
    PerspectiveFrustum _perspectiveFrustum{};
    OrthoFrustum _orthoFrustum{};
    Vec3f _position{0.0f};
    Vec3f _forward{0.0f, 0.0f, 1.0f};
    Vec3f _up{0.0f, 1.0f, 0.0f};
    Quaternion _orientation{};
    Mat4 _attitude{1.0f};
    Mat4 _projectionMat{1.0f};
    bool _dirty{false};
};

} // namespace raum::scene