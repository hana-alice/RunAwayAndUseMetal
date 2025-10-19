#include "VKGraphicsPipeline.h"
#include "VKDevice.h"
#include "VKPipelineLayout.h"
#include "VKRenderPass.h"
#include "VKShader.h"
#include "VKUtils.h"
namespace raum::rhi {

namespace {



}

GraphicsPipeline::GraphicsPipeline(const GraphicsPipelineInfo& pipelineInfo, Device* device)
: RHIGraphicsPipeline(pipelineInfo, device), 
_device(static_cast<Device*>(device)), 
_layout(static_cast<PipelineLayout*>(pipelineInfo.pipelineLayout)) {
    RAUM_ERROR_IF(pipelineInfo.shaders.size() < 2, "At least two shaders are required!");

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (auto* rhiSHader : pipelineInfo.shaders) {
        auto* shader = static_cast<Shader*>(rhiSHader);
        if (shader->stage() == ShaderStage::VERTEX) {
            VkPipelineShaderStageCreateInfo vertexStage{};
            vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertexStage.pName = "main";
            vertexStage.module = shader->shaderModule();
            vertexStage.pSpecializationInfo = nullptr;
            shaderStages.emplace_back(vertexStage);
        } else if (shader->stage() == ShaderStage::TASK) {
            VkPipelineShaderStageCreateInfo taskStage{};
            taskStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            taskStage.stage = VK_SHADER_STAGE_TASK_BIT_EXT;
            taskStage.pName = "main";
            taskStage.module = shader->shaderModule();
            taskStage.pSpecializationInfo = nullptr;
            shaderStages.emplace_back(taskStage);
        } else if (shader->stage() == ShaderStage::MESH) {
            VkPipelineShaderStageCreateInfo meshStage{};
            meshStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            meshStage.stage = VK_SHADER_STAGE_MESH_BIT_EXT;
            meshStage.pName = "main";
            meshStage.module = shader->shaderModule();
            meshStage.pSpecializationInfo = nullptr;
            shaderStages.emplace_back(meshStage);
        } else if (shader->stage() == ShaderStage::FRAGMENT) {
            VkPipelineShaderStageCreateInfo fragmentStage{};
            fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragmentStage.pName = "main";
            fragmentStage.module = shader->shaderModule();
            fragmentStage.pSpecializationInfo = nullptr;
            shaderStages.emplace_back(fragmentStage);
        }
    }
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineCreateInfo.pStages = shaderStages.data();

    std::vector<VkDynamicState> dynamicStates{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        // VK_DYNAMIC_STATE_LINE_WIDTH,
        // VK_DYNAMIC_STATE_DEPTH_BIAS,
        // VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        // VK_DYNAMIC_STATE_DEPTH_BOUNDS,
        // VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
        // VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
        // VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        // VK_DYNAMIC_STATE_VIEWPORT_W_SCALING_NV,
        // VK_DYNAMIC_STATE_DISCARD_RECTANGLE_EXT,
        // VK_DYNAMIC_STATE_SAMPLE_LOCATIONS_EXT,
        // VK_DYNAMIC_STATE_VIEWPORT_SHADING_RATE_PALETTE_NV,
        // VK_DYNAMIC_STATE_VIEWPORT_COARSE_SAMPLE_ORDER_NV,
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    pipelineCreateInfo.pDynamicState = &dynamicState;

    const auto& vertexLayout = pipelineInfo.vertexLayout;
    std::vector<VkVertexInputBindingDescription> bindingDescs(vertexLayout.vertexBufferAttrs.size());
    std::vector<VkVertexInputAttributeDescription> attrDescs(vertexLayout.vertexAttrs.size());

    for (size_t i = 0; i < vertexLayout.vertexBufferAttrs.size(); ++i) {
        auto& bindingDesc = bindingDescs[i];
        const auto& vbAttr = vertexLayout.vertexBufferAttrs[i];
        bindingDesc.binding = vbAttr.binding;
        bindingDesc.stride = vbAttr.stride;
        bindingDesc.inputRate = mapRate(vbAttr.rate);
    }

    for (size_t i = 0; i < vertexLayout.vertexAttrs.size(); ++i) {
        const auto& vertexAttr = vertexLayout.vertexAttrs[i];
        auto& attrDesc = attrDescs[i];
        attrDesc.binding = vertexAttr.binding;
        attrDesc.location = vertexAttr.location;
        attrDesc.offset = vertexAttr.offset;
        attrDesc.format = formatInfo(vertexAttr.format).format;
    }

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size());
    vertexInputState.pVertexBindingDescriptions = bindingDescs.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInputState.pVertexAttributeDescriptions = attrDescs.data();
    pipelineCreateInfo.pVertexInputState = &vertexInputState;

    VkPipelineInputAssemblyStateCreateInfo iaInfo{};
    iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    iaInfo.topology = primitiveTopology(pipelineInfo.primitiveType);
    iaInfo.primitiveRestartEnable = VK_FALSE;
    pipelineCreateInfo.pInputAssemblyState = &iaInfo;

    VkPipelineViewportStateCreateInfo vpInfo{};
    vpInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vpInfo.viewportCount = pipelineInfo.viewportCount;
    vpInfo.scissorCount = pipelineInfo.viewportCount;
    pipelineCreateInfo.pViewportState = &vpInfo;

    const RasterizationInfo& rasterizationInfo = pipelineInfo.rasterizationInfo;
    VkPipelineRasterizationStateCreateInfo rsInfo{};
    rsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rsInfo.cullMode = cullMode(rasterizationInfo.cullMode);
    rsInfo.lineWidth = rasterizationInfo.lineWidth;
    rsInfo.polygonMode = polygonMode(rasterizationInfo.polygonMode);
    rsInfo.depthClampEnable = rasterizationInfo.depthClamp;
    rsInfo.depthBiasEnable = rasterizationInfo.depthBiasEnable;
    rsInfo.depthBiasClamp = rasterizationInfo.depthBiasClamp;
    rsInfo.depthBiasConstantFactor = rasterizationInfo.depthBiasConstantFactor;
    rsInfo.depthBiasSlopeFactor = rasterizationInfo.depthBiasSlopeFactor;
    pipelineCreateInfo.pRasterizationState = &rsInfo;

    const MultisamplingInfo& multisamplingInfo = pipelineInfo.multisamplingInfo;
    VkPipelineMultisampleStateCreateInfo msInfo{};
    msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msInfo.alphaToCoverageEnable = multisamplingInfo.alphaToCoverageEnable;
    msInfo.minSampleShading = multisamplingInfo.minSampleShading;
    msInfo.pSampleMask = &multisamplingInfo.sampleMask;
    msInfo.sampleShadingEnable = multisamplingInfo.sampleShadingEnable;
    msInfo.rasterizationSamples = sampleCount(multisamplingInfo.sampleCount);
    pipelineCreateInfo.pMultisampleState = &msInfo;

    const DepthStencilInfo& depthStencilInfo = pipelineInfo.depthStencilInfo;
    VkPipelineDepthStencilStateCreateInfo dsInfo{};
    dsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dsInfo.depthTestEnable = depthStencilInfo.depthTestEnable;
    dsInfo.depthWriteEnable = depthStencilInfo.depthWriteEnable;
    dsInfo.depthCompareOp = compareOp(depthStencilInfo.depthCompareOp);
    dsInfo.depthBoundsTestEnable = depthStencilInfo.depthBoundsTestEnable;
    dsInfo.minDepthBounds = depthStencilInfo.minDepthBounds;
    dsInfo.maxDepthBounds = depthStencilInfo.maxDepthBounds;
    dsInfo.stencilTestEnable = depthStencilInfo.stencilTestEnable;

    dsInfo.front.depthFailOp = stencilOp(depthStencilInfo.front.depthFailOp);
    dsInfo.front.failOp = stencilOp(depthStencilInfo.front.failOp);
    dsInfo.front.passOp = stencilOp(depthStencilInfo.front.passOp);
    dsInfo.front.compareOp = compareOp(depthStencilInfo.front.compareOp);
    dsInfo.front.compareMask = depthStencilInfo.front.compareMask;
    dsInfo.front.writeMask = depthStencilInfo.front.writeMask;
    dsInfo.front.reference = depthStencilInfo.front.reference;

    dsInfo.back.depthFailOp = stencilOp(depthStencilInfo.back.depthFailOp);
    dsInfo.back.failOp = stencilOp(depthStencilInfo.back.failOp);
    dsInfo.back.passOp = stencilOp(depthStencilInfo.back.passOp);
    dsInfo.back.compareOp = compareOp(depthStencilInfo.back.compareOp);
    dsInfo.back.compareMask = depthStencilInfo.back.compareMask;
    dsInfo.back.writeMask = depthStencilInfo.back.writeMask;
    dsInfo.back.reference = depthStencilInfo.back.reference;

    dsInfo.minDepthBounds = depthStencilInfo.minDepthBounds;
    dsInfo.maxDepthBounds = depthStencilInfo.maxDepthBounds;

    pipelineCreateInfo.pDepthStencilState = &dsInfo;

    const auto& colorBlendInfo = pipelineInfo.colorBlendInfo;
    VkPipelineColorBlendStateCreateInfo cbInfo{};
    cbInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbInfo.blendConstants[0] = colorBlendInfo.blendConstants[0];
    cbInfo.blendConstants[1] = colorBlendInfo.blendConstants[1];
    cbInfo.blendConstants[2] = colorBlendInfo.blendConstants[2];
    cbInfo.blendConstants[3] = colorBlendInfo.blendConstants[3];
    cbInfo.logicOpEnable = colorBlendInfo.logicOpEnable;
    cbInfo.logicOp = logicOp(colorBlendInfo.logicOp);
    cbInfo.attachmentCount = static_cast<uint32_t>(colorBlendInfo.attachmentBlends.size());
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(colorBlendInfo.attachmentBlends.size());
    for (size_t i = 0; i < colorBlendInfo.attachmentBlends.size(); ++i) {
        const auto& attachmentBlend = colorBlendInfo.attachmentBlends[i];
        auto& colorBlendAttachment = colorBlendAttachments[i];
        colorBlendAttachment.blendEnable = attachmentBlend.blendEnable;
        colorBlendAttachment.srcColorBlendFactor = blendFactor(attachmentBlend.srcColorBlendFactor);
        colorBlendAttachment.dstColorBlendFactor = blendFactor(attachmentBlend.dstColorBlendFactor);
        colorBlendAttachment.srcAlphaBlendFactor = blendFactor(attachmentBlend.srcAlphaBlendFactor);
        colorBlendAttachment.dstAlphaBlendFactor = blendFactor(attachmentBlend.dstAlphaBlendFactor);
        colorBlendAttachment.alphaBlendOp = blendOp(attachmentBlend.alphaBlendOp);
        colorBlendAttachment.colorWriteMask = colorComponentFlags(attachmentBlend.writemask);
    }
    cbInfo.pAttachments = colorBlendAttachments.data();
    pipelineCreateInfo.pColorBlendState = &cbInfo;
    pipelineCreateInfo.subpass = pipelineInfo.subpassIndex;
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = 0;
    pipelineCreateInfo.layout = static_cast<PipelineLayout*>(pipelineInfo.pipelineLayout)->layout();
    pipelineCreateInfo.renderPass = static_cast<RenderPass*>(pipelineInfo.renderPass)->renderPass();

    VkResult res = vkCreateGraphicsPipelines(_device->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_pipeline);
}

GraphicsPipeline::~GraphicsPipeline() {
    vkDestroyPipeline(_device->device(), _pipeline, nullptr);
}

}; // namespace raum::rhi