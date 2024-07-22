#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIImage: public RHIResource  {
public:
    explicit RHIImage(const ImageInfo& info, RHIDevice*) : _info(info) {}
    ImageInfo info() const { return _info; }

    virtual ~RHIImage() = 0;

protected:
    ImageInfo _info;
};

inline RHIImage::~RHIImage() {}

} // namespace raum::rhi