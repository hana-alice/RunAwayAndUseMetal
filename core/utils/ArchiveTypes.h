#pragma once

#include "core/math.h"

#include "cereal/cereal.hpp"
#include "cereal/types/string.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/variant.hpp"
#include "cereal/types/memory.hpp"
#include <cereal/archives/binary.hpp>
namespace cereal {

template <class Archive>
void serialize(Archive& ar, raum::Mat2& mat2) {
    for (int i = 0; i < 4; i++) {
        ar(mat2[i]);
    }
}

template <class Archive>
void serialize(Archive& ar, raum::Mat3& mat3) {
    for (int i = 0; i < 9; i++) {
        ar(mat3[i]);
    }
}

template <class Archive>
void serialize(Archive& ar, raum::Mat4& mat4) {
    for (int i = 0; i < 16; i++) {
        ar(mat4[i]);
    }
}

template <class Archive>
void serialize(Archive& ar, raum::Vec2i& vec2i) { ar(vec2i.x, vec2i.y); }

template <class Archive>
void serialize(Archive& ar, raum::Vec3i& vec3i) { ar(vec3i.x, vec3i.y, vec3i.z); }

template <class Archive>
void serialize(Archive& ar, raum::Vec4i& vec4i) { ar(vec4i.x, vec4i.y, vec4i.z, vec4i.w); }

template <class Archive>
void serialize(Archive& ar, raum::Vec2u& vec2u) { ar(vec2u.x, vec2u.y); }

template <class Archive>
void serialize(Archive& ar, raum::Vec3u& vec3u) { ar(vec3u.x, vec3u.y, vec3u.z); }

template <class Archive>
void serialize(Archive& ar, raum::Vec4u& vec4u) { ar(vec4u.x, vec4u.y, vec4u.z, vec4u.w); }

template <class Archive>
void serialize(Archive& ar, raum::Vec2f& vec2f) { ar(vec2f.x, vec2f.y); }

template <class Archive>
void serialize(Archive& ar, raum::Vec3f& vec3f) { ar(vec3f.x, vec3f.y, vec3f.z); }

template <class Archive>
void serialize(Archive& ar, raum::Vec4f& vec4f) { ar(vec4f.x, vec4f.y, vec4f.z, vec4f.w); }

template <class Archive>
void serialize(Archive& ar, raum::Quaternion& quat) { ar(quat.x, quat.y, quat.z, quat.w); }

template <class Archive>
void serialize(Archive& ar, raum::utils::Degree& degree) {
    ar(degree.value);
}

template <class Archive>
void serialize(Archive& ar, raum::utils::Radian& rad) {
    ar(rad.value);
}

// template <class Archive, typename Enum>
// void serialize(Archive& ar, Enum& e)
//     requires std::is_enum<Enum>::value
// {
//     ar(static_cast<std::underlying_type_t<Enum>>(e));
// }

// template <class Archive, typename... Tpyes>
// void serialize(Archive& ar, std::variant<Tpyes...>& var) {
//     ar(var.index());
//
//     std::visit([&](auto&& arg) {
//         ar(arg);
//     },
//                var);
// }

} // namespace raum