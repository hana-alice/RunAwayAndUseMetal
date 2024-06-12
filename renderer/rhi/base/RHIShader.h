#pragma once
#include "RHIDefine.h"
#include "RHIResource.h"
namespace raum::rhi {
class RHIDevice;
class RHIShader: public RHIResource  {
public:
    explicit RHIShader(const ShaderBinaryInfo&, RHIDevice*) {}
    explicit RHIShader(const ShaderSourceInfo&, RHIDevice*) {}

    virtual ~RHIShader() = 0;

protected:
};

inline RHIShader::~RHIShader() {}

} // namespace raum::rhi