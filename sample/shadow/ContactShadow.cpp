#include "ContactShadow.h"

#include "BuiltinRes.h"
#include "Pipeline.h"
#include "bend_sss_cpu.h"
#include "core/utils/utils.h"

namespace raum::sample {
namespace {

constexpr Vec4f LightPosition{3.0f, 3.0f, 3.0f, 0.0f};

} // namespace

void ContactShadowSample::onInitialize() {
    const auto& resourcePath = utils::resourceDirectory();
    loadScene(resourcePath / "models" / "DamagedHelmet" / "DamagedHelmet.gltf");

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
        .nearPlane = 50.0f,
        .farPlane = 0.1f,
        .position = {0.0f, 0.0f, 4.0f},
        .yawDegrees = 180.0f,
    });
    enableFlyCamera({.moveSpeed = 3.0f});

    const scene::OrthoFrustum shadowFrustum{-5.0f, 5.0f, -5.0f, 5.0f, 20.0f, 0.1f};
    _shadowCamera = std::make_shared<scene::Camera>(shadowFrustum);
    auto& shadowEye = _shadowCamera->eye();
    shadowEye.setPosition(5.0f, 5.0f, 5.0f);
    shadowEye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    shadowEye.update();

    ensureSwapchain("present");
    ensureViewportImage(
        "forwardDepth",
        rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT | rhi::ImageUsage::SAMPLED,
        rhi::Format::D32_SFLOAT_S8_UINT);
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
        rhi::Format::D32_SFLOAT_S8_UINT);
    ensureCameraResources(false);
    ensureBuffer(
        "shadowView",
        2 * sizeof(Mat4) + sizeof(Vec2f),
        rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST);
    ensureSampler("shadowSampler", rhi::SamplerInfo{
                                       .magFilter = rhi::Filter::NEAREST,
                                       .minFilter = rhi::Filter::NEAREST,
                                       .mipmapMode = rhi::MipmapMode::NEAREST,
                                       .addressModeU = rhi::SamplerAddressMode::CLAMP_TO_BORDER,
                                       .addressModeV = rhi::SamplerAddressMode::CLAMP_TO_BORDER,
                                       .addressModeW = rhi::SamplerAddressMode::CLAMP_TO_BORDER,
                                   });

    ensureBuffer(
        "contactShadowInfo",
        3 * sizeof(Vec2f),
        rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST);
    for (size_t index = 0; index < _waveOffsetBuffers.size(); ++index) {
        const auto localName = "waveOffset" + std::to_string(index);
        ensureBuffer(
            localName,
            sizeof(Vec2f),
            rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST);
        _waveOffsetBuffers[index] = resource(localName);
    }
    ensureViewportImage(
        "contactShadow",
        rhi::ImageUsage::STORAGE | rhi::ImageUsage::SAMPLED,
        rhi::Format::R32_SFLOAT);
    ensureBuffer(
        "viewportSize",
        sizeof(Vec2f),
        rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST);
}

void ContactShadowSample::onRender() {
    auto& renderGraph = pipeline().renderGraph();
    resizeViewportResources();

    {
        auto uploadPass = renderGraph.addCopyPass("viewportUpdate");
        const Vec2f viewportSize{
            static_cast<float>(viewportWidth()),
            static_cast<float>(viewportHeight()),
        };
        uploadPass.uploadBuffer(
            &viewportSize[0],
            sizeof(Vec2f),
            resource("viewportSize"),
            0);
    }

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
                0.0f,
                0);
        shadowPass.addQueue("shadowMap")
            .setViewport(0, 0, ShadowMapWidth, ShadowMapHeight, 0.0f, 1.0f)
            .addFlag(graph::RenderQueueFlags::REVERSE_Z)
            .addCamera(_shadowCamera.get())
            .addUniformBuffer(resource("shadowView"), "Mat");
    }

    {
        auto uploadPass = renderGraph.addCopyPass("cameraBufferUpdate");
        uploadCamera(uploadPass, false);
    }

    {
        auto depthPass = renderGraph.addRenderPass("depthPrePass");
        depthPass.addDepthStencil(
            resource("forwardDepth"),
            graph::LoadOp::CLEAR,
            graph::StoreOp::STORE,
            graph::LoadOp::DONT_CARE,
            graph::StoreOp::DONT_CARE,
            0.0f,
            0);
        depthPass.addQueue("DepthOnly")
            .setViewport(0, 0, viewportWidth(), viewportHeight(), 0.0f, 1.0f)
            .addFlag(graph::RenderQueueFlags::REVERSE_Z)
            .addCamera(&camera())
            .addUniformBuffer(cameraBuffer(), "Mat");
    }

    {
        const auto& eye = camera().eye();
        const auto lightProjection = eye.projection() * eye.attitude() * LightPosition;
        int viewport[2] = {
            static_cast<int>(viewportWidth()),
            static_cast<int>(viewportHeight()),
        };
        int minimumBounds[2] = {0, 0};
        int maximumBounds[2] = {viewport[0], viewport[1]};
        float projectedLight[4] = {
            lightProjection.x,
            lightProjection.y,
            lightProjection.z,
            lightProjection.w,
        };
        const auto dispatchList = Bend::BuildDispatchList(
            projectedLight,
            viewport,
            minimumBounds,
            maximumBounds,
            false,
            64);

        auto uploadPass = renderGraph.addCopyPass("contactShadowUpdate");
        for (size_t index = 0; index < dispatchList.DispatchCount; ++index) {
            const auto& dispatch = dispatchList.Dispatch[index];
            const Vec2f offset{
                static_cast<float>(dispatch.WaveOffset_Shader[0]),
                static_cast<float>(dispatch.WaveOffset_Shader[1]),
            };
            uploadPass.uploadBuffer(
                &offset[0],
                sizeof(Vec2f),
                _waveOffsetBuffers[index],
                0);
        }
        uploadPass.uploadBuffer(
            &dispatchList.LightCoordinate_Shader[0],
            sizeof(Vec4f),
            resource("contactShadowInfo"),
            0);
        const Vec2f inverseDepthSize{
            1.0f / static_cast<float>(viewportWidth()),
            1.0f / static_cast<float>(viewportHeight()),
        };
        uploadPass.uploadBuffer(
            &inverseDepthSize[0],
            sizeof(Vec2f),
            resource("contactShadowInfo"),
            sizeof(Vec4f));

        for (size_t index = 0; index < dispatchList.DispatchCount; ++index) {
            const auto& dispatch = dispatchList.Dispatch[index];
            renderGraph.addComputePass("contactShadow" + std::to_string(index))
                .setProgramName("asset/layout/ContactShadowBendCS")
                .setDispatch(dispatch.WaveCount[0], dispatch.WaveCount[1], dispatch.WaveCount[2])
                .addSampledDepth(resource("forwardDepth"), "DepthTexture")
                .addResource(resource("shadowSampler"), "DepthTextureSampler", graph::Access::READ)
                .addResource(resource("contactShadowInfo"), "UniformInfoBuffer", graph::Access::READ)
                .addResource(resource("contactShadow"), "OutputTexture", graph::Access::WRITE)
                .addResource(_waveOffsetBuffers[index], "WaveOffsetsBuffer", graph::Access::READ);
        }
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
            0.0f,
            0);
    renderPass.addQueue("solid_with_ao")
        .setViewport(0, 0, viewportWidth(), viewportHeight(), 0.0f, 1.0f)
        .addCamera(&camera())
        .addFlag(graph::RenderQueueFlags::REVERSE_Z)
        .addUniformBuffer(cameraBuffer(), "Mat")
        .addUniformBuffer(resource("shadowView"), "ShadowView")
        .addSampledImage(resource("shadowColor"), "shadowMap")
        .addSampler(resource("shadowSampler"), "shadowSampler")
        .addSampledImage(resource("contactShadow"), "ao")
        .addUniformBuffer(resource("viewportSize"), "ViewportSize");
}

} // namespace raum::sample
