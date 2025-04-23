#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <memory>
#include "core/math.h"
#include "core/utils/utils.h"

namespace raum::scene {

namespace Coord {
constexpr Vec3f X{1.0f, 0.0f, 0.0f};
constexpr Vec3f Y{0.0f, 1.0f, 0.0f};
constexpr Vec3f Z{0.0f, 0.0f, 1.0f};
} // namespace Coord

class Renderable {
public:
    Renderable();
    virtual ~Renderable() {}

private:
    uint64_t objectID{0};
};
using RenderablePtr = std::shared_ptr<Renderable>;

enum class Projection {
    PERSPECTIVE,
    ORTHOGRAPHIC,
};

struct PerspectiveFrustum {
    utils::Degree fov{45.0f};   // degree
    float aspect{0.0f}; // eg. 900.0f/600.0f = 1.5f
    float near{0.1f};
    float far{1000.0f};
};

struct OrthoFrustum {
    float left{0.0f};
    float right{0.0f};
    float bottom{0.0f};
    float top{0.0f};
    float near{0.1f};
    float far{1000.0f};
};

struct AABB {
    Vec3f minBound{};
    Vec3f maxBound{};
};

} // namespace raum::scene