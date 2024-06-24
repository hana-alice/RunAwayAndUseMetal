#pragma once
#include "ShaderGraph.h"
#include "Skybox.h"
namespace raum::asset {

class BuiltinRes {
public:
    static void initialize(graph::ShaderGraph& shaderGraph,rhi::DevicePtr device);

    static rhi::ImagePtr iblBrdfLUT();
    static rhi::ImageViewPtr iblBrdfLUTView();

    static const Skybox& skybox();

};

}