#include "VKGraphicsPipeline.h"
#include "VKShader.h"
#include "VKUtils.h"
#include "log.h"
namespace raum::rhi {
GraphicsPipelineState::GraphicsPipelineState(const GraphicsPipelineStateInfo& pipelineInfo) {
    RAUM_ERROR_IF(pipelineInfo.shaders.size() < 2, "At least two shaders are required!");

    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(pipelineInfo.shaders.size());
    for (auto* shader : pipelineInfo.shaders) {
        if (shader->stage() == ShaderStage::VERTEX) {
            VkPipelineShaderStageCreateInfo vertexStage{};
            vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertexStage.pName = "main";
            vertexStage.module = shader->shaderModule();
            vertexStage.pSpecializationInfo = nullptr;
            shaderStages.emplace_back(vertexStage);
        } else if (shader->stage() == ShaderStage::TASK) {
        } else if (shader->stage() == ShaderStage::MESH) {
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
    dynamicState.dynamicStateCount = dynamicStates.size();
    dynamicState.pDynamicStates = dynamicStates.data();

    const auto& vertexLayout = pipelineInfo.vertexLayout;
    std::vector<VkVertexInputBindingDescription> bindingDescs(vertexLayout.size());
    std::vector<VkVertexInputAttributeDescription> attrDescs{};
    std::vector<uint32_t> offset(vertexLayout.size(), 0);
    for (size_t i = 0; i < vertexLayout.size(); ++i) {
        const auto& bufferLayout = vertexLayout[i];
        auto& bindingDesc = bindingDescs[i];
        bindingDesc.inputRate = mapRate(bufferLayout.front().rate);
        // TODO(Zeqiang): Required in feature implementation
        bindingDesc.binding = i;
        for (size_t j = 0; j < bufferLayout.size(); ++j) {
            const auto& desc = bufferLayout[j];
            const auto& fmtInfo = formatInfo(desc.format);
            auto& attr = attrDescs.emplace_back();
            attr.binding = desc.binding;
            attr.location = desc.location;
            attr.format = fmtInfo.format;
            attr.offset = offset[attr.binding];
            offset[attr.binding] += fmtInfo.size;
        }
    }
    for (size_t i = 0; i < vertexLayout.size(); ++i) {
        auto& bindingDesc = bindingDescs[i];
        bindingDesc.stride = offset[i];
    }

    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescs.size());
    vertexInputState.pVertexBindingDescriptions = bindingDescs.data();
    vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
    vertexInputState.pVertexAttributeDescriptions = attrDescs.data();
}
}; // namespace raum::rhi