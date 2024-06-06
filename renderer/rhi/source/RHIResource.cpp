#include "RHIResource.h"

namespace raum::rhi {

static uint64_t s_objectID{0};

RHIResource::RHIResource():_objectID(++s_objectID) {}

uint64_t RHIResource::objectID() const {
    return _objectID;
}

}