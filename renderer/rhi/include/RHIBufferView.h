#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIBufferView : public RHIResource {
public:
    explicit RHIBufferView(const BufferViewInfo& info, RHIDevice* device) {}
    virtual ~RHIBufferView() = 0;
};

inline RHIBufferView::~RHIBufferView() {}

} // namespace raum::rhi