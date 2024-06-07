#include "Scene.h"
#include "Common.h"

namespace raum::scene {

static uint64_t object_id{0};

Renderable::Renderable(): objectID(++object_id) {
}

}