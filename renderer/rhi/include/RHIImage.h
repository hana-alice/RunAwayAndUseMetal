#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIImage {
public:
    explicit RHIImage(const ImageInfo&, RHIDevice*) {}

protected:
    virtual ~RHIImage() = 0;
};

inline RHIImage::~RHIImage() {}

} // namespace raum::rhi