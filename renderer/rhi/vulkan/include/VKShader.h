#pragma once
#include "VKDefine.h"
#include "shaderc/shaderc.hpp"
namespace raum::rhi {
class Shader {
public:
    explicit Shader(const ShaderSourceInfo& shaderInfo);
    explicit Shader(const ShaderBinaryInfo& shaderInfo);
    Shader() = delete;
    Shader(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    ~Shader();

    ShaderStage stage() const { return _stage; }

    VkShaderModule shaderModule() const { return _shaderModule; }

private:
    VkShaderModule _shaderModule;
    static shaderc::Compiler shaderCompiler;

    ShaderStage _stage;
    DEBUG(std::string _source;)
};

} // namespace raum::rhi