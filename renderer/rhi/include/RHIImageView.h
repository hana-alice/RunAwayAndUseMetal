#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIImage;
class RHIImageView : public RHIResource {
public:
    explicit RHIImageView(const ImageViewInfo& info, RHIDevice*) : _info(info) {}
    virtual RHIImage* image() const = 0;
    ImageViewInfo viewInfo() const { return _info; }

    virtual ~RHIImageView() = 0;

protected:
    const ImageViewInfo _info;
};

inline RHIImageView::~RHIImageView() {}

} // namespace raum::rhi