#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIImageView {
public:
    explicit RHIImageView(const ImageViewInfo&, RHIDevice*) {}
    virtual ~RHIImageView() = 0;
};
} // namespace raum::rhi