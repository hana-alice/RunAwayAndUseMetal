#include "BuiltinRes.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHIUtils.h"
#include "Serialization.h"
#include "core/utils/utils.h"
#include "stb_image.h"
#include "BRDFLUT.h"
namespace raum::asset {

rhi::ImagePtr s_iblBrdfLUT;
rhi::ImageViewPtr s_iblBrdfLUTView;

Skybox* s_skybox = nullptr;

void BuiltinRes::initialize(graph::ShaderGraph& shaderGraph, rhi::DevicePtr device) {
    // deserialize layout(json): shader description
    const auto& resourcePath = utils::resourceDirectory();
    graph::deserialize(resourcePath / "shader", "cook-torrance", shaderGraph);
    graph::deserialize(resourcePath / "shader", "skybox", shaderGraph);
    shaderGraph.compile("asset");

    auto cmdPool = rhi::CommandPoolPtr(device->createCoomandPool({}));
    auto cmdBuffer = rhi::CommandBufferPtr(cmdPool->makeCommandBuffer({}));
    auto* queue = device->getQueue({rhi::QueueType::GRAPHICS});
    cmdBuffer->enqueue(queue);
    cmdBuffer->begin({});

    // brdf lut
    {
        auto [img, view] = generateBRDFLUT(cmdBuffer, device);
        s_iblBrdfLUT = img;
        s_iblBrdfLUTView = view;
    }

    // skybox
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
        cmdBuffer->onComplete([imgBuffer]() mutable {
            imgBuffer.reset();
        });
    }

    cmdBuffer->commit();
    queue->submit();
}

const Skybox& BuiltinRes::skybox() {
    return *s_skybox;
}

rhi::ImagePtr BuiltinRes::iblBrdfLUT() {
    return s_iblBrdfLUT;
}

rhi::ImageViewPtr BuiltinRes::iblBrdfLUTView() {
    return s_iblBrdfLUTView;
}

} // namespace raum::asset