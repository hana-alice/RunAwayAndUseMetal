#include "VKRenderPass.h"
#include "VKDevice.h"
#include "VKUtils.h"
#include "log.h"
namespace raum::rhi {

RenderPass::RenderPass(const RenderPassInfo& info, RHIDevice* device)
: RHIRenderPass(info, device), _device(static_cast<Device*>(device)) {
    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

    std::vector<VkAttachmentDescription> attachments(info.attachments.size());
    for (size_t i = 0; i < info.attachments.size(); ++i) {
        auto& attachment = attachments[i];
        const auto& attachmentDesc = info.attachments[i];
        attachment.format = formatInfo(attachmentDesc.format).format;
        attachment.samples = sampleCount(attachmentDesc.sampleCount);
        attachment.loadOp = loadOp(attachmentDesc.loadOp);
        attachment.storeOp = storeOp(attachmentDesc.storeOp);
        attachment.stencilLoadOp = loadOp(attachmentDesc.stencilLoadOp);
        attachment.stencilStoreOp = storeOp(attachmentDesc.stencilStoreOp);
        attachment.initialLayout = imageLayout(attachmentDesc.initialLayout);
        attachment.finalLayout = imageLayout(attachmentDesc.finalLayout);
    }
    createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    createInfo.pAttachments = attachments.data();

    std::vector<VkSubpassDescription> subpasses(info.subpasses.size());
    std::vector<std::vector<VkAttachmentReference>> subpassInputs(info.subpasses.size());
    std::vector<std::vector<VkAttachmentReference>> subpassColors(info.subpasses.size());
    std::vector<std::vector<VkAttachmentReference>> subpassResolves(info.subpasses.size());
    std::vector<std::vector<VkAttachmentReference>> subpassDepthStencil(info.subpasses.size());
    for (size_t i = 0; i < info.subpasses.size(); ++i) {
        auto& subpass = subpasses[i];
        const auto& subpassDesc = info.subpasses[i];
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        for (const auto& input : subpassDesc.inputs) {
            subpassInputs[i].emplace_back(VkAttachmentReference{input.index, imageLayout(input.layout)});
        }
        subpass.inputAttachmentCount = static_cast<uint32_t>(subpassInputs[i].size());
        subpass.pInputAttachments = subpassInputs[i].data();

        for (const auto& color : subpassDesc.colors) {
            subpassColors[i].emplace_back(VkAttachmentReference{color.index, imageLayout(color.layout)});
        }
        subpass.colorAttachmentCount = static_cast<uint32_t>(subpassColors[i].size());
        subpass.pColorAttachments = subpassColors[i].data();

        for (const auto& ds : subpassDesc.depthStencil) {
            subpassDepthStencil[i].emplace_back(VkAttachmentReference{ds.index, imageLayout(ds.layout)});
        }
        subpass.pDepthStencilAttachment = subpassDepthStencil[i].data();

        for (const auto& resolve : subpassDesc.resolves) {
            subpassResolves[i].emplace_back(VkAttachmentReference{resolve.index, imageLayout(resolve.layout)});
        }
        subpass.pResolveAttachments = subpassResolves[i].data();

        subpass.preserveAttachmentCount = static_cast<uint32_t>(subpassDesc.preserves.size());
        subpass.pPreserveAttachments = subpassDesc.preserves.data();
    }
    createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
    createInfo.pSubpasses = subpasses.data();

    std::vector<VkSubpassDependency> dependencies(info.dependencies.size());
    for (size_t i = 0; i < info.dependencies.size(); ++i) {
        const auto& dependencyDesc = info.dependencies[i];
        auto& dependency = dependencies[i];
        dependency.srcSubpass = dependencyDesc.src;
        dependency.dstSubpass = dependencyDesc.dst;
        dependency.srcStageMask = pipelineStageFlags(dependencyDesc.srcStage);
        dependency.dstStageMask = pipelineStageFlags(dependencyDesc.dstStage);
        dependency.srcAccessMask = accessFlags(dependencyDesc.srcAccessFlags);
        dependency.dstAccessMask = accessFlags(dependencyDesc.dstAccessFlags);
        dependency.dependencyFlags = dependencyFlags(dependencyDesc.dependencyFlags);
    }
    createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    createInfo.pDependencies = dependencies.data();

    VkResult res = vkCreateRenderPass(_device->device(), &createInfo, nullptr, &_renderPass);
    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create renderpass");
}

RenderPass::~RenderPass() {
    vkDestroyRenderPass(_device->device(), _renderPass, nullptr);
}

} // namespace raum::rhi