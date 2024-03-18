#include "Scene.h"
#include "Common.h"

namespace raum::scene {

static uint64_t object_id{0};

Renderobject::Renderobject(): objectID(++object_id) {
}

}