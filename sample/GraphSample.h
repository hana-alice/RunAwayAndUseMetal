#pragma once

#include "BuiltinRes.h"
#include "CameraSample.h"
#include "Pipeline.h"
#include "core/utils/utils.h"

namespace raum::sample {

class GraphSample final : public CameraSample {
public:
    explicit GraphSample(framework::Director* director)
    : CameraSample(director, "GraphSample") {}

private:
    void onInitialize() override {
        const auto& resourcePath = utils::resourceDirectory();
        loadScene(resourcePath / "models" / "sponza" / "sponza.gltf");

        const auto& skyboxName = resource("skybox");
        auto& skyboxNode = sceneGraph().addModel(skyboxName);
        skyboxNode.model = asset::BuiltinRes::skybox().model();
        trackSceneNode(skyboxName);

        createPerspectiveCamera({
            .verticalFov = utils::Degree{45.0f},
            .nearPlane = 0.01f,
            .farPlane = 10.0f,
            .position = {0.0f, 0.0f, 4.0f},
            .yawDegrees = 180.0f,
        });
        enableFlyCamera({.moveSpeed = 10.1f});

        ensureSwapchain("present");
        ensureViewportImage(
            "forwardDepth",
            rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT,
            rhi::Format::D24_UNORM_S8_UINT);
        ensureCameraResources();
        ensureLightResource();
    }

    void onRender() override {
        auto& renderGraph = pipeline().renderGraph();
        resizeViewportResources();

        auto uploadPass = renderGraph.addCopyPass("cameraBufferUpdate");
        uploadCamera(uploadPass);
        uploadLight(uploadPass, Vec4f{5.0f, 5.0f, 0.0f, 1.0f});

        auto renderPass = renderGraph.addRenderPass("forward");
        renderPass
            .addColor(resource("present"), graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.3f, 0.3f, 0.3f, 1.0f})
            .addDepthStencil(
                resource("forwardDepth"),
                graph::LoadOp::CLEAR,
                graph::StoreOp::STORE,
                graph::LoadOp::CLEAR,
                graph::StoreOp::STORE,
                1.0f,
                0);
        renderPass.addQueue("default")
            .setViewport(0, 0, viewportWidth(), viewportHeight(), 0.0f, 1.0f)
            .addCamera(&camera())
            .addUniformBuffer(cameraBuffer(), "Mat")
            .addUniformBuffer(cameraPositionBuffer(), "CamPos")
            .addUniformBuffer(lightBuffer(), "Light");
    }
};

} // namespace raum::sample
