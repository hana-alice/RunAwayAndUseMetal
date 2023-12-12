#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIShader {
public:
    explicit RHIShader(const ShaderBinaryInfo&) {}
    explicit RHIShader(const ShaderSourceInfo&) {}
    virtual ~RHIShader() = 0;
};
} // namespace raum::rhi