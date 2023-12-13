#pragma once
#include "RHIDefine.h"
namespace raum::rhi {
class RHIDevice;
class RHIShader {
public:
    explicit RHIShader(const ShaderBinaryInfo&, RHIDevice*) {}
    explicit RHIShader(const ShaderSourceInfo&, RHIDevice*) {}

protected:
    virtual ~RHIShader() = 0;
};

inline RHIShader::~RHIShader() {}

} // namespace raum::rhi