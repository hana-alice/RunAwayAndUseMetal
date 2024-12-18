#pragma once
#include "RHIBufferView.h"
#include "VKDefine.h"
namespace raum::rhi {
class Device;
class BufferView : public RHIBufferView {
public:
    BufferView() = delete;
    BufferView(const BufferView&) = delete;
    BufferView(BufferView&&) = delete;
    BufferView& operator=(const BufferView&) = delete;

    explicit BufferView(const BufferViewInfo& info, RHIDevice* device);
    ~BufferView();

    VkBufferView bufferView() const { return _bufferView; }

private:
    Device* _device{nullptr};
    VkBufferView _bufferView;
};
} // namespace raum::rhi