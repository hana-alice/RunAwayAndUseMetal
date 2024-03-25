#pragma once
#include <memory>

namespace raum::scene {

class Light {
public:
    Light() = default;
};

using LightPtr = std::shared_ptr<Light>;

}