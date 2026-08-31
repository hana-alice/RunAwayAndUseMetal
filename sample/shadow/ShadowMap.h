#pragma once

#include "../CameraSample.h"
#include "BuiltinRes.h"
#include "Pipeline.h"
#include "SceneSerializer.h"
#include "core/utils/utils.h"

namespace raum::sample {

class ShadowMapSample final : public CameraSample {
public:
    explicit ShadowMapSample(framework::Director* director)
    : CameraSample(director, "ShadowMap") {}

private:
    static constexpr uint32_t ShadowMapWidth = 1024;
    static constexpr uint32_t ShadowMapHeight = 1024;

    void onInitialize() override {
        const auto& quadName = resource("quad");
        auto& quadNode = sceneGraph().addModel(quadName);
        quadNode.model = asset::BuiltinRes::quad().model()->createInstance();
        trackSceneNode(quadName);

        auto& quadRenderer = quadNode.model->meshRenderers().front();
        const auto scale = glm::scale(Mat4{1.0f}, Vec3f{3.0f, 3.0f, 3.0f});
        const auto rotation = glm::rotate(Mat4{1.0f}, glm::radians(-90.0f), Vec3f{1.0f, 0.0f, 0.0f});
        const auto translation = glm::translate(Mat4{1.0f}, Vec3f{0.0f, -1.0f, 0.0f});
        quadRenderer->setTransform(translation * rotation * scale);

        createPerspectiveCamera({
            .verticalFov = utils::Degree{45.0f},
            .nearPlane = 0.1f,
            .farPlane = 50.0f,
            .position = {0.0f, 0.0f, 4.0f},
            .yawDegrees = 180.0f,
        });
        enableFlyCamera({.moveSpeed = 3.0f});

        const scene::OrthoFrustum shadowFrustum{-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 20.0f};
        _shadowCamera = std::make_shared<scene::Camera>(shadowFrustum);
        auto& shadowEye = _shadowCamera->eye();
        shadowEye.setPosition(5.0f, 5.0f, 5.0f);
        shadowEye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        shadowEye.update();

        ensureSwapchain("present");
        ensureViewportImage(
            "forwardDepth",
            rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT,
            rhi::Format::D24_UNORM_S8_UINT);
        ensureImage(
            "shadowColor",
            rhi::ImageUsage::COLOR_ATTACHMENT | rhi::ImageUsage::SAMPLED,
            ShadowMapWidth,
            ShadowMapHeight,
            rhi::Format::R32_SFLOAT);
        ensureImage(
            "shadowDepth",
            rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT,
            ShadowMapWidth,
            ShadowMapHeight,
            rhi::Format::D24_UNORM_S8_UINT);
        ensureCameraResources();
        ensureLightResource();
        ensureBuffer(
            "shadowView",
            2 * sizeof(Mat4) + sizeof(Vec2f),
            rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST);
        ensureSampler("shadowSampler", rhi::SamplerInfo{
                                           .magFilter = rhi::Filter::LINEAR,
                                           .minFilter = rhi::Filter::LINEAR,
                                           .mipmapMode = rhi::MipmapMode::NEAREST,
                                           .addressModeU = rhi::SamplerAddressMode::CLAMP_TO_EDGE,
                                           .addressModeV = rhi::SamplerAddressMode::CLAMP_TO_EDGE,
                                           .addressModeW = rhi::SamplerAddressMode::CLAMP_TO_EDGE,
                                       });
    }

    void onRender() override {
        auto& renderGraph = pipeline().renderGraph();
        resizeViewportResources();

        {
            auto uploadPass = renderGraph.addCopyPass("shadowCameraUpdate");
            const auto& shadowEye = _shadowCamera->eye();
            uploadPass.uploadBuffer(&shadowEye.attitude()[0], sizeof(Mat4), resource("shadowView"), 0);
            uploadPass.uploadBuffer(&shadowEye.projection()[0], sizeof(Mat4), resource("shadowView"), sizeof(Mat4));
            constexpr Vec2f inverseShadowSize{
                1.0f / static_cast<float>(ShadowMapWidth),
                1.0f / static_cast<float>(ShadowMapHeight),
            };
            uploadPass.uploadBuffer(
                &inverseShadowSize[0],
                sizeof(Vec2f),
                resource("shadowView"),
                2 * sizeof(Mat4));
        }

        {
            auto shadowPass = renderGraph.addRenderPass("shadowMap");
            shadowPass
                .addColor(resource("shadowColor"), graph::LoadOp::CLEAR, graph::StoreOp::STORE, {1.0f})
                .addDepthStencil(
                    resource("shadowDepth"),
                    graph::LoadOp::CLEAR,
                    graph::StoreOp::DONT_CARE,
                    graph::LoadOp::DONT_CARE,
                    graph::StoreOp::DONT_CARE,
                    1.0f,
                    0);
            shadowPass.addQueue("shadowMap")
                .setViewport(0, 0, ShadowMapWidth, ShadowMapHeight, 0.0f, 1.0f)
                .addCamera(_shadowCamera.get())
                .addUniformBuffer(resource("shadowView"), "Mat");
        }

        {
            auto uploadPass = renderGraph.addCopyPass("cameraBufferUpdate");
            uploadCamera(uploadPass);
            uploadLight(uploadPass);
        }

        auto renderPass = renderGraph.addRenderPass("forward");
        renderPass
            .addColor(resource("present"), graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.2f, 0.4f, 0.4f, 1.0f})
            .addDepthStencil(
                resource("forwardDepth"),
                graph::LoadOp::CLEAR,
                graph::StoreOp::DONT_CARE,
                graph::LoadOp::DONT_CARE,
                graph::StoreOp::DONT_CARE,
                1.0f,
                0);
        renderPass.addQueue("solidColor")
            .setViewport(0, 0, viewportWidth(), viewportHeight(), 0.0f, 1.0f)
            .addCamera(&camera())
            .addUniformBuffer(cameraBuffer(), "Mat")
            .addUniformBuffer(resource("shadowView"), "ShadowView")
            .addSampledImage(resource("shadowColor"), "shadowMap")
            .addSampler(resource("shadowSampler"), "shadowSampler");
    }

    scene::CameraPtr _shadowCamera;
};

} // namespace raum::sample
