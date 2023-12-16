#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIImage;
class RHIImageView {
public:
    explicit RHIImageView(const ImageViewInfo& info, RHIDevice*): _info(info) {}
    virtual RHIImage* image() const = 0;
    ImageViewInfo viewInfo() const { return _info; }

protected:
    virtual ~RHIImageView() = 0;
    const ImageViewInfo _info;
};

inline RHIImageView::~RHIImageView() {}

} // namespace raum::rhi