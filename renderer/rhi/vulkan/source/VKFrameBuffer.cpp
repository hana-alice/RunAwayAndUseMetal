#include "VKFrameBuffer.h"
#include "VKDevice.h"
#include "VKRenderPass.h"
#include "VKImageView.h"
#include "log.h"
namespace raum::rhi {
FrameBuffer::FrameBuffer(const FrameBufferInfo& info, RHIDevice* device)
: RHIFrameBuffer(info, device), _device(static_cast<Device*>(device)) {
    VkFramebufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.width = info.width;
    createInfo.height = info.height;
    createInfo.layers = info.layers;
    createInfo.renderPass = static_cast<RenderPass*>(info.renderPass)->renderPass();
    std::vector<VkImageView> imageViews(info.images.size());
    for (size_t i = 0; i < info.images.size(); ++i) {
        imageViews[i] = static_cast<ImageView*>(info.images[i])->imageView();
    }
    createInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
    createInfo.pAttachments = imageViews.data();

    VkResult res = vkCreateFramebuffer(_device->device(), &createInfo, nullptr, &_framebuffer);
    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create framebuffer");
}

FrameBuffer::~FrameBuffer() {
    vkDestroyFramebuffer(_device->device(), _framebuffer, nullptr);
}
}