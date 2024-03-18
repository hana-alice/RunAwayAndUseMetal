#pragma once
#include "core/math.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

namespace raum::scene {

class Renderobject {
public:
    Renderobject();
    virtual ~Renderobject() {}

private:
    uint64_t objectID{0};
};


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
    float fov{45.0f};   // radians
    float aspect{0.0f}; // eg. 900.0f/600.0f = 1.5f
    float near{0.1f};
    float far{1000.0f};
};

} // namespace raum::scene