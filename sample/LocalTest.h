#pragma once

#include "BuiltinRes.h"
#include "CameraSample.h"
#include "PBRMaterial.h"
#include "Pipeline.h"
#include "RHIUtils.h"
#include "core/utils/utils.h"

namespace raum::sample {

class LocalTestSample final : public CameraSample {
public:
    explicit LocalTestSample(framework::Director* director)
    : CameraSample(director, "LocalTest") {}

private:
    void onLoad(const LoadingProgressCallback& progress) override {
        const auto& resourcePath = utils::resourceDirectory();
        _sceneBounds = loadScene(resourcePath / "models" / "sponza" / "sponza.gltf", "scene", progress);
    }

    void onInitialize() override {
        const auto cameraPosition = _sceneBounds
            ? (_sceneBounds->minBound + _sceneBounds->maxBound) * 0.5f
            : Vec3f{0.0f, 0.0f, 50.0f};
        createPerspectiveCamera({
            .verticalFov = utils::Degree{60.0f},
            .nearPlane = 1.0f,
            .farPlane = 1000.0f,
            .position = cameraPosition,
            .yawDegrees = 180.0f,
        });
        enableFlyCamera({.moveSpeed = 18.0f});

        {
            auto materialTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/fullscreen/rasterBlit");
            auto quadMaterial = materialTemplate->instantiate("asset/layout/fullscreen/rasterBlit", scene::MaterialType::CUSTOM);
            quadMaterial->set("mainTexture", scene::Texture{
                                                 .texture = rhi::defaultSampledImage(device()),
                                                 .textureView = rhi::defaultSampledImageView(device()),
                                             });
            quadMaterial->set("mainSampler", scene::Sampler{rhi::SamplerInfo{
                                                 .magFilter = rhi::Filter::LINEAR,
                                                 .minFilter = rhi::Filter::LINEAR,
                                             }});

            _rasterBlitTechnique = std::make_shared<scene::Technique>(quadMaterial, "default");
            _rasterBlitTechnique->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
            _rasterBlitTechnique->blendInfo().attachmentBlends.emplace_back();
        }

        const auto& skyboxName = resource("skybox");
        auto& skyboxNode = sceneGraph().addModel(skyboxName);
        skyboxNode.model = asset::BuiltinRes::skybox().model();
        skyboxNode.hint = graph::ModelHint::NO_CULLING;
        trackSceneNode(skyboxName);

        ensureSwapchain("present");
        ensureViewportImage(
            "forwardDepth",
            rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT,
            rhi::Format::D24_UNORM_S8_UINT);
        ensureViewportImage(
            "forwardColor",
            rhi::ImageUsage::COLOR_ATTACHMENT | rhi::ImageUsage::SAMPLED,
            rhi::Format::BGRA8_UNORM);
        ensureCameraResources();
        ensureLightResource();
    }

    void onRender() override {
        auto& renderGraph = pipeline().renderGraph();
        resizeViewportResources();

        auto uploadPass = renderGraph.addCopyPass("cameraBufferUpdate");
        uploadCamera(uploadPass);
        uploadLight(uploadPass, Vec4f{5.0f, 5.0f, 0.0f, 1.0f});

        auto forwardPass = renderGraph.addRenderPass("forward");
        forwardPass
            .addColor(resource("forwardColor"), graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.3f, 0.3f, 0.3f, 1.0f})
            .addDepthStencil(
                resource("forwardDepth"),
                graph::LoadOp::CLEAR,
                graph::StoreOp::STORE,
                graph::LoadOp::CLEAR,
                graph::StoreOp::STORE,
                1.0f,
                0);
        forwardPass.addQueue("default")
            .setViewport(0, 0, viewportWidth(), viewportHeight(), 0.0f, 1.0f)
            .addFlag(graph::RenderQueueFlags::GEOMETRY)
            .addCamera(&camera())
            .addUniformBuffer(cameraBuffer(), "Mat")
            .addUniformBuffer(cameraPositionBuffer(), "CamPos")
            .addUniformBuffer(lightBuffer(), "Light");

        auto presentPass = renderGraph.addRenderPass("present");
        presentPass.addColor(resource("present"), graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.3f, 0.3f, 0.3f, 1.0f});
        presentPass.addQueue("default")
            .setViewport(0, 0, viewportWidth(), viewportHeight(), 0.0f, 1.0f)
            .setQuadTech(_rasterBlitTechnique)
            .addSampledImage(resource("forwardColor"), "mainTexture");
    }

    scene::TechniquePtr _rasterBlitTechnique;
    std::optional<scene::AABB> _sceneBounds;
};

} // namespace raum::sample
