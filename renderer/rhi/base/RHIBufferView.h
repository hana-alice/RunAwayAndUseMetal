#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIBufferView : public RHIResource {
public:
    explicit RHIBufferView(const BufferViewInfo& info, RHIDevice* device) {}
    virtual ~RHIBufferView() = 0;

    virtual const BufferViewInfo& info() const = 0;
};

inline RHIBufferView::~RHIBufferView() {}

} // namespace raum::rhi