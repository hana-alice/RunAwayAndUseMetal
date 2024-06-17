#pragma once
#include "Model.h"
#include "RHICommandPool.h"
#include "RHIImage.h"
#include "ShaderGraph.h"
namespace raum::asset {

class Skybox {
public:
    Skybox() = delete;
    Skybox(rhi::BufferPtr hdrData,
           uint32_t width,
           uint32_t height,
           rhi::CommandBufferPtr cmdBuffer,
           rhi::DevicePtr device,
           graph::ShaderGraph& shaderGraph);
    Skybox(rhi::ImagePtr cubeMap,
           rhi::CommandBufferPtr cmdBuffer,
           rhi::DevicePtr device,
           graph::ShaderGraph& shaderGraph);

    scene::ModelPtr model() const;

    rhi::ImagePtr diffuseIrradianceImage() const;
    rhi::ImageViewPtr diffuseIrradianceView() const;

    rhi::ImagePtr specularIrradianceImage() const;
    rhi::ImageViewPtr specularIrradianceView() const;

private:
    void init(scene::MeshRendererPtr meshRenderer, rhi::CommandBufferPtr cmdBuffer, rhi::DevicePtr device);

    rhi::ImagePtr _cubeImage;
    rhi::ImageViewPtr _cubeView;

    // ibl
    rhi::ImagePtr _diffuse;
    rhi::ImageViewPtr _diffuseView;
    rhi::ImagePtr _specular;
    rhi::ImageViewPtr _specularView;

    scene::ModelPtr _model;
};

} // namespace raum::asset