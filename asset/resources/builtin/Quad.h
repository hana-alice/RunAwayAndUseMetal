#pragma once
#include "Model.h"
#include "RHICommandPool.h"
#include "RHIImage.h"
#include "ShaderGraph.h"
namespace raum::asset {

class Quad {
public:
    Quad() = delete;
    Quad(rhi::CommandBufferPtr cmdBuffer,
         rhi::DevicePtr device,
         graph::ShaderGraph& shaderGraph);

    scene::ModelPtr model() const;

private:

    scene::ModelPtr _model;
};

} // namespace raum::asset