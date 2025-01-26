#include "BuiltinRes.h"
#include "BRDFLUT.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHIUtils.h"
#include "Serialization.h"
#include "core/utils/utils.h"
#include "stb_image.h"
namespace raum::asset {

rhi::ImagePtr s_iblBrdfLUT;
rhi::ImageViewPtr s_iblBrdfLUTView;

Skybox* s_skybox = nullptr;
Quad* s_quad = nullptr;

void defaultResourceTransition(rhi::CommandBufferPtr commandBuffer, rhi::DevicePtr device) {
    auto sampledImage = rhi::defaultSampledImage(device);
    rhi::ImageBarrierInfo transition{
        .image = sampledImage.get(),
        .dstStage = rhi::PipelineStage::VERTEX_SHADER,
        .newLayout = rhi::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
        .dstAccessFlag = rhi::AccessFlags::SHADER_READ,
        .range = {
            .aspect = rhi::AspectMask::COLOR,
            .sliceCount = 1,
            .mipCount = 1,
        }};
    commandBuffer->appendImageBarrier(transition);

    auto storageImage = rhi::defaultStorageImage(device);
    transition.image = storageImage.get();
    transition.newLayout = rhi::ImageLayout::GENERAL;
    transition.dstAccessFlag = rhi::AccessFlags::SHADER_WRITE;
    transition.dstStage = rhi::PipelineStage::VERTEX_SHADER | rhi::PipelineStage::COMPUTE_SHADER;
    commandBuffer->appendImageBarrier(transition);
    commandBuffer->applyBarrier({});
}

void BuiltinRes::initialize(graph::ShaderGraph& shaderGraph, rhi::DevicePtr device) {
    // deserialize layout(json): shader description
    const auto& resourcePath = utils::resourceDirectory();
    graph::deserialize(resourcePath / "shader", "cook-torrance", shaderGraph);
    graph::deserialize(resourcePath / "shader", "skybox", shaderGraph);
    graph::deserialize(resourcePath / "shader", "simple", shaderGraph);
    graph::deserialize(resourcePath / "shader", "sparse", shaderGraph);
    graph::deserialize(resourcePath / "shader", "ShadowMap", shaderGraph);
    graph::deserialize(resourcePath / "shader", "solidColor", shaderGraph);
    graph::deserialize(resourcePath / "shader", "contactShadow", shaderGraph);
    graph::deserialize(resourcePath / "shader", "DepthOnly", shaderGraph);
    shaderGraph.compile("asset");

    auto cmdPool = rhi::CommandPoolPtr(device->createCoomandPool({}));
    auto cmdBuffer = rhi::CommandBufferPtr(cmdPool->makeCommandBuffer({}));
    auto* queue = device->getQueue({rhi::QueueType::GRAPHICS});
    cmdBuffer->enqueue(queue);
    cmdBuffer->begin({});

    defaultResourceTransition(cmdBuffer, device);

    // brdf lut
    {
        auto [img, view] = generateBRDFLUT(cmdBuffer, device);
        s_iblBrdfLUT = img;
        s_iblBrdfLUTView = view;
    }

    // skybox, quad
    {
        int width, height, nrComponents;
        auto imgFile = resourcePath / "skies" / "wrestling_gym_4k.hdr";
        float* data = stbi_loadf(imgFile.string().c_str(), &width, &height, &nrComponents, 4);
        rhi::BufferSourceInfo bufInfo{
            .bufferUsage = rhi::BufferUsage::TRANSFER_DST | rhi::BufferUsage::TRANSFER_SRC,
            .size = static_cast<uint32_t>(width * height * 4 * sizeof(float)),
            .data = data};
        auto imgBuffer = rhi::BufferPtr(device->createBuffer(bufInfo));
        stbi_image_free(data);
        s_skybox = new Skybox(imgBuffer, width, height, cmdBuffer, device, shaderGraph);

        s_quad = new Quad(cmdBuffer, device);

        cmdBuffer->onComplete([imgBuffer]() mutable {
            imgBuffer.reset();
        });
    }

    cmdBuffer->commit();
    queue->submit(false);
}

const Skybox& BuiltinRes::skybox() {
    return *s_skybox;
}

const Quad& BuiltinRes::quad() {
    return *s_quad;
}

rhi::ImagePtr BuiltinRes::iblBrdfLUT() {
    return s_iblBrdfLUT;
}

rhi::ImageViewPtr BuiltinRes::iblBrdfLUTView() {
    return s_iblBrdfLUTView;
}

} // namespace raum::asset