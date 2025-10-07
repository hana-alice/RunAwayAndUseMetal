#pragma once
#include <glm/glm.hpp>

namespace raum {
using Vec2i = glm::ivec2;
using Vec3i = glm::ivec3;
using Vec4i = glm::ivec4;
using Vec2u = glm::uvec2;
using Vec3u = glm::uvec3;
using Vec4u = glm::uvec4;
using Vec2f = glm::vec2;
using Vec3f = glm::vec3;
using Vec4f = glm::vec4;
using Mat2 = glm::mat2;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

using Quaternion = glm::quat;

const float FloatEpsilon = 1e-5f;
const double DoubleEpsilon = 1e-10;

template <typename T>
struct Epsilon {
    constexpr static T value = std::numeric_limits<T>::epsilon();
};

template<>
struct Epsilon<float> {
    constexpr static float value = FloatEpsilon;
};

template<>
struct Epsilon<double> {
    constexpr static double value = DoubleEpsilon;
};

template <typename T>
bool realEqual(T a, T b) {
    return std::abs(a - b) < Epsilon<T>::value;
}

}