#include "Quad.h"
#include "GeometryMesh.h"
#include "RHIUtils.h"

namespace raum::asset {

Quad::Quad(rhi::CommandBufferPtr cmdBuffer, rhi::DevicePtr device, graph::ShaderGraph &shaderGraph) {
    _model = std::make_shared<scene::Model>();
    auto mesh = std::make_shared<scene::Mesh>();
    auto& meshData = mesh->meshData();
    if (!scene::Quad::vertexBuffer.buffer) {
        rhi::BufferSourceInfo info{
            .bufferUsage = rhi::BufferUsage::VERTEX | rhi::BufferUsage::TRANSFER_DST,
            .size = static_cast<uint32_t>(scene::Quad::vertices.size() * sizeof(float)),
            .data = scene::Quad::vertices.data(),
        };

        scene::Quad::vertexBuffer.buffer = rhi::BufferPtr(device->createBuffer(info));
    }
    meshData.vertexBuffer.buffer = scene::Quad::vertexBuffer.buffer;

    meshData.shaderAttrs = scene::ShaderAttribute::POSITION | scene::ShaderAttribute::UV;
    meshData.vertexLayout.vertexBufferAttrs.emplace_back(0, static_cast<uint32_t>(5 * sizeof(float)));
    meshData.vertexLayout.vertexAttrs.emplace_back(0, 0, rhi::Format::RGB32_SFLOAT);
    meshData.vertexLayout.vertexAttrs.emplace_back(1, 0, rhi::Format::RG32_SFLOAT, static_cast<uint32_t>(3 * sizeof(float)));
    meshData.vertexCount = 6;

    auto meshRenderer = _model->meshRenderers().emplace_back(std::make_shared<scene::MeshRenderer>(mesh));

    scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/simple");
    auto quadMat = matTemplate->instantiate("asset/layout/simple", scene::MaterialType::CUSTOM);

    scene::Texture tex {
        .texture = rhi::defaultSampledImage(device),
        .textureView = rhi::defaultSampledImageView(device),
    };
    quadMat->set("mainTexture", tex);
    scene::Sampler sampler{
        rhi::SamplerInfo{
            .magFilter = rhi::Filter::LINEAR,
            .minFilter = rhi::Filter::LINEAR,
        },
    };
    quadMat->set("mainSampler", sampler);

    scene::TechniquePtr quadTech = std::make_shared<scene::Technique>(quadMat, "default");
    quadTech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
    auto& ds = quadTech->depthStencilInfo();
    ds.depthTestEnable = true;
    ds.depthWriteEnable = false;
    ds.depthCompareOp = rhi::CompareOp::LESS_OR_EQUAL;
    auto& bs = quadTech->blendInfo();
    bs.attachmentBlends.emplace_back();

    meshRenderer->addTechnique(quadTech);
    meshRenderer->setVertexInfo(0, 6, 0);
}

scene::ModelPtr Quad::model() const {
    return _model;
}

}