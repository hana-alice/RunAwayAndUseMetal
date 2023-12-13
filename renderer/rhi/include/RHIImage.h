#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIImage {
public:
    explicit RHIImage(const ImageInfo&, RHIDevice*) {}
    virtual ~RHIImage() = 0;
};
} // namespace raum::rhi