#pragma once
#include "RHIShader.h"
#include "VKDefine.h"
#include "shaderc/shaderc.hpp"
namespace raum::rhi {
class Device;
class Shader : public RHIShader {
public:
    explicit Shader(const ShaderSourceInfo& shaderInfo, RHIDevice* device);
    explicit Shader(const ShaderBinaryInfo& shaderInfo, RHIDevice* device);
    Shader() = delete;
    Shader(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    ~Shader();

    ShaderStage stage() const { return _stage; }

    VkShaderModule shaderModule() const { return _shaderModule; }

private:
    VkShaderModule _shaderModule;

    Device* _device{nullptr};
    ShaderStage _stage;
    DEBUG(std::string _source;)
};

} // namespace raum::rhi