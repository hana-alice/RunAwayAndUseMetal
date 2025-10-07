#include "VKBufferView.h"
#include "VKBuffer.h"
#include "VKDevice.h"
#include "VKUtils.h"
namespace raum::rhi {

BufferView::BufferView(const BufferViewInfo& info, RHIDevice* device)
: RHIBufferView(info, device), _device(static_cast<Device*>(device)), _info(info) {
    VkBufferViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    createInfo.buffer = static_cast<Buffer*>(info.buffer)->buffer();
    createInfo.format = formatInfo(info.format).format;
    createInfo.offset = info.offset;
    createInfo.range = info.size;

    vkCreateBufferView(_device->device(), &createInfo, nullptr, &_bufferView);
}

BufferView::~BufferView() {
    vkDestroyBufferView(_device->device(), _bufferView, nullptr);
}

const BufferViewInfo& BufferView::info() const {
    return _info;
}


} // namespace raum::rhi