#pragma once
#include <stdint.h>
#include <vector>
#include "RHIDefine.h"
namespace raum::scene {
static uint64_t object_id{0};
class Renderobject {
public:
    Renderobject() : objectID(++object_id) {}
    virtual ~Renderobject() {}

private:
    uint64_t objectID{0};
};

} // namespace raum::scene