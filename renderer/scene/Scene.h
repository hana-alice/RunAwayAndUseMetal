#pragma once
#include <stdint.h>
#include <vector>
#include "RHIDefine.h"
#include "Model.h"
namespace raum::scene {

class Scene {
public:
    Scene() {};

    std::vector<Model> models;
};

} // namespace raum::scene