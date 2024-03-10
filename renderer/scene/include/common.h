#pragma once
// #include "define.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace raum::scene {

using Vec3i = glm::ivec3;
using Vec4i = glm::ivec4;
using Vec3u = glm::uvec3;
using Vec4u = glm::uvec4;
using Vec3f = glm::vec3;
using Vec4f = glm::vec4;
using Quaternion = glm::quat;
using Mat4 = glm::mat4;
using Mat3 = glm::mat3;

struct Degree {
    float value{0.0};
};

struct Radian {
    float value{0.0};
};

enum class Projection {
    PERSPECTIVE,
    ORTHOGRAPHIC,
};

struct Frustum {
    float fov{0.0f};
    float aspect{0.0f};
    float near{0.0f};
    float far{0.0f};
};

} // namespace raum::scene