#include "VKComputePipeline.h"
#include "VKDevice.h"
#include "VKPipelineLayout.h"
#include "VKShader.h"
#include "VKUtils.h"
namespace raum::rhi {
ComputePipeline::ComputePipeline(const ComputePipelineInfo& info, Device* device)
: RHIComputePipeline(info, device),
  _device(static_cast<Device*>(device)),
  _layout(static_cast<PipelineLayout*>(info.pipelineLayout)) {
    VkPipelineShaderStageCreateInfo shaderStage = {};
    shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module = static_cast<Shader*>(info.shader)->shaderModule();
    shaderStage.pName = "main";
    shaderStage.pSpecializationInfo = nullptr;

    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stage = shaderStage;
    pipelineCreateInfo.layout = _layout->layout();
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreateInfo.basePipelineIndex = 0;

    VkPipeline pipeline;
    VK_CHECK_RESULT(vkCreateComputePipelines(_device->device(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline));
    _pipeline = pipeline;
}

ComputePipeline::~ComputePipeline() {
    vkDestroyPipeline(_device->device(), _pipeline, nullptr);
}

} // namespace raum::rhi
