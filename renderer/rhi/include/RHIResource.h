#pragma once
#include <stdint.h>
namespace raum::rhi {

class RHIResource {
public:
    RHIResource();
    uint64_t objectID() const;

    virtual ~RHIResource() {}

private:
    uint64_t _objectID;
};

} // namespace raum::rhi