#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIImageView {
public:
    explicit RHIImageView(const ImageViewInfo&, RHIDevice*) {}

protected:
    virtual ~RHIImageView() = 0;
};

inline RHIImageView::~RHIImageView() {}

} // namespace raum::rhi