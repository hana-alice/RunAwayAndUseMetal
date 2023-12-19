#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIImage {
public:
    explicit RHIImage(const ImageInfo& info, RHIDevice*) : _info(info) {}
    ImageInfo info() const { return _info; }

    virtual ~RHIImage() = 0;

protected:
    const ImageInfo _info;
};

inline RHIImage::~RHIImage() {}

} // namespace raum::rhi