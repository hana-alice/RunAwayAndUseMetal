#include "VKShader.h"
#include "VKDevice.h"
#include "VKUtils.h"
#include "shaderc/shaderc.h"
#include "shaderc/shaderc.hpp"

namespace raum::rhi {
Shader::Shader(const ShaderSourceInfo& shaderInfo, RHIDevice* device)
: RHIShader(shaderInfo, device), _device(static_cast<Device*>(device)) {
    _stage = shaderInfo.stage.stage;
    DEBUG(_source = shaderInfo.stage.source;)

    shaderc::Compiler shaderCompiler{};
    shaderc::CompileOptions options{};
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_3);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    options.SetGenerateDebugInfo();

    // P. B. T
    auto mapStage = [](ShaderStage stage) {
        switch (stage) {
            case ShaderStage::VERTEX:
                return shaderc_glsl_vertex_shader;
            case ShaderStage::FRAGMENT:
                return shaderc_glsl_fragment_shader;
            case ShaderStage::COMPUTE:
                return shaderc_glsl_compute_shader;
        }
        return shaderc_glsl_vertex_shader;
    };

    const auto& stage = shaderInfo.stage;

    auto result = shaderCompiler.CompileGlslToSpv(stage.source.c_str(), stage.source.size(), mapStage(stage.stage), shaderInfo.sourcePath.c_str(), "main", options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        RAUM_ERROR("Failed to compile shader: {0}", result.GetErrorMessage());
    }

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = (result.end() - result.begin())*4;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(result.cbegin());
    VkResult res = vkCreateShaderModule(_device->device(), &createInfo, nullptr, &_shaderModule);

    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create shader module");
}

Shader::Shader(const raum::rhi::ShaderBinaryInfo& shaderInfo, raum::rhi::RHIDevice* device)
: RHIShader(shaderInfo, device), _device(static_cast<Device*>(device)) {
    _stage = shaderInfo.stage.stage;


    const auto& stage = shaderInfo.stage;

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderInfo.stage.spv.size() * 4;
    createInfo.pCode = shaderInfo.stage.spv.data();
    VkResult res = vkCreateShaderModule(_device->device(), &createInfo, nullptr, &_shaderModule);

    RAUM_ERROR_IF(res != VK_SUCCESS, "Failed to create shader module");
}

Shader::~Shader() {
    vkDestroyShaderModule(_device->device(), _shaderModule, nullptr);
}

} // namespace raum::rhi