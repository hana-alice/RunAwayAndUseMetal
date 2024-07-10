#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIImage;
class RHIImageView : public RHIResource {
public:
    explicit RHIImageView(const ImageViewInfo& info, RHIDevice*){}
    explicit RHIImageView(const SparseImageViewInfo& info, RHIDevice*) {}
    virtual RHIImage* image() const = 0;

    virtual ~RHIImageView() = 0;

protected:
};

inline RHIImageView::~RHIImageView() {}

} // namespace raum::rhi