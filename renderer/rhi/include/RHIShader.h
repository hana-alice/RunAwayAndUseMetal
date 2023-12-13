#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIShader {
public:
    explicit RHIShader(const ShaderBinaryInfo&, RHIDevice*) {}
    explicit RHIShader(const ShaderSourceInfo&, RHIDevice*) {}
    virtual ~RHIShader() = 0;
};
} // namespace raum::rhi