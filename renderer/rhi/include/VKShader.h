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

private:
    VkShaderModule _shaderModule;
    static shaderc::Compiler shaderCompiler;
};

} // namespace raum::rhi