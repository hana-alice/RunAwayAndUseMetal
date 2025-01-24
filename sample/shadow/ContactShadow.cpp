#include "ContactShadow.h"
#include "bend_sss_cpu.h"

namespace raum::sample {

constexpr bool ENABLE_SHADOWMAP{false};

constexpr Vec4f LightPos{3.0, 3.0, 3.0, 0.0};

ContactShadowSample::~ContactShadowSample() {
    _keyListener.remove();
    _mouseListener.remove();
}

void ContactShadowSample::hide() {
    _director->sceneGraph().disable(_name);
}

void ContactShadowSample::show() {
    auto& renderGraph = _ppl->renderGraph();
    _ppl->resourceGraph().updateImage("forwardDS", _swapchain->width(), _swapchain->height());

    if constexpr (ENABLE_SHADOWMAP) {
        // shadow buffer upload pass
        {
            auto uploadPass = renderGraph.addCopyPass("shadowCamUpdate");
            auto& shadowEye = _shadowCam->eye();
            const auto& shadowViewMat = shadowEye.inverseAttitude();
            uploadPass.uploadBuffer(&shadowViewMat[0], 64, _shadowVPBuffer, 0);
            const auto& shadowProjMat = shadowEye.projection();
            uploadPass.uploadBuffer(&shadowProjMat[0], 64, _shadowVPBuffer, 64);
            constexpr float invSize[2] = {1.0f / shadowMapWidth, 1.0f / shadowMapHeight};
            uploadPass.uploadBuffer(&invSize[0], 8, _shadowVPBuffer, 128);
        }

        // shadow rendering pass
        {
            auto shadowPass = renderGraph.addRenderPass("shadowMap");
            shadowPass.addColor(_shadowMapRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {1.0f})
                .addDepthStencil(_shadowMapDS, graph::LoadOp::CLEAR, graph::StoreOp::DONT_CARE, graph::LoadOp::DONT_CARE, graph::StoreOp::DONT_CARE, 1.0, 0);
            auto shadowQ = shadowPass.addQueue("shadowMap");
            shadowQ.setViewport(0, 0, shadowMapWidth, shadowMapHeight, 0.0f, 1.0f)
                .addCamera(_shadowCam.get())
                .addUniformBuffer(_shadowVPBuffer, "Mat");
        }
    }

    // main camera upload pass
    {
        auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");
        auto& eye = _cam->eye();
        auto viewMat = eye.inverseAttitude();
        uploadPass.uploadBuffer(&viewMat[0], 64, _camBuffer, 0);
        const auto& projMat = eye.projection();
        uploadPass.uploadBuffer(&projMat[0], 64, _camBuffer, 64);

        uploadPass.uploadBuffer(&eye.getPosition()[0], 12, _camPose, 0);
        Vec4f color{1.0, 1.0, 1.0, 1.0};
        Vec4f lightPos{LightPos.x, LightPos.y, LightPos.z, 1.0};
        uploadPass.uploadBuffer(&lightPos[0], 16, _light, 0);
        uploadPass.uploadBuffer(&color[0], 16, _light, 16);
    }

    auto width = _swapchain->width();
    auto height = _swapchain->height();
    // depth pre-pass
    {
        auto depthPrePass = renderGraph.addRenderPass("depthPrePass");
        depthPrePass.addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::STORE, graph::LoadOp::DONT_CARE, graph::StoreOp::DONT_CARE, 1.0, 0);
        auto depthPrePassQ = depthPrePass.addQueue("DepthOnly");
        depthPrePassQ.setViewport(0, 0, width, height, 0.0f, 1.0f)
            .addCamera(_cam.get())
            .addUniformBuffer(_camBuffer, "Mat");
    }

    // compute contact shadow
    {
        auto& eye = _cam->eye();
        auto viewMat = eye.inverseAttitude();
        const auto& projMat = eye.projection();
        const auto& lightProj = LightPos * viewMat * projMat;

        int viewport[2] = {static_cast<int>(width), static_cast<int>(height)};
        int minBounds[2] = {0, 0};
        int maxBounds[2] = {static_cast<int>(width), static_cast<int>(height)};
        float lightProjArray[4] = {lightProj.x, lightProj.y, lightProj.z, lightProj.w};
        const auto& dispatchList = Bend::BuildDispatchList(lightProjArray, viewport, minBounds, maxBounds, false, 64);
        for (size_t i = 0; i < dispatchList.DispatchCount; ++i) {
            auto& dispatch = dispatchList.Dispatch[i];
            auto contactShadowPass = renderGraph.addComputePass("contactShadow" + std::to_string(i));
            contactShadowPass.setProgramName("asset/layout/ContactShadowBendCS")
                .setDispatch(64, 1, 1)
                .addSampledDepth(_forwardDS, "DepthTexture")
                .addResource(_shadowSampler, "DepthTextureSampler", graph::Access::READ)
                .addResource(_sssInfo, "UniformInfoBuffer", graph::Access::READ)
                .addResource(_sssOuput, "OutputTexture", graph::Access::WRITE)
                .addResource(_waveOffsetBuffer, "WaveOffsetsBuffer", graph::Access::READ);
        }
    }

    // rendering
    {
        auto renderPass = renderGraph.addRenderPass("forward");

        renderPass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.2, 0.4, 0.4, 1.0})
            .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::DONT_CARE, graph::LoadOp::DONT_CARE, graph::StoreOp::DONT_CARE, 1.0, 0);
        auto queue = renderPass.addQueue("solidColor");

        auto width = _swapchain->width();
        auto height = _swapchain->height();
        queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
            .addCamera(_cam.get())
            .addUniformBuffer(_camBuffer, "Mat")
            .addUniformBuffer(_shadowVPBuffer, "ShadowView")
            .addSampledImage(_shadowMapRT, "shadowMap")
            .addSampler(_shadowSampler, "shadowSampler");
    }
}

void ContactShadowSample::init() {
    // load scene from gltf
    _device = _director->device();
    _swapchain = _director->swapchain();
    const auto& resourcePath = utils::resourceDirectory();

    // load models
    {
        auto& sceneGraph = _director->sceneGraph();
        asset::serialize::load(sceneGraph, resourcePath / "models" / "DamagedHelmet" / "DamagedHelmet.gltf", _device);

        auto& quad = asset::BuiltinRes::quad();
        graph::ModelNode& quadNode = sceneGraph.addModel("quad");
        quadNode.model = quad.model();
        auto& quadRenderer = quadNode.model->meshRenderers().front();

        auto scaleMat = glm::scale(Mat4{1.0f}, Vec3f{3.0f, 3.0f, 3.0f});
        auto rotMat = glm::rotate(Mat4{1.0f}, glm::radians(-90.0f), Vec3f{1.0f, 0.0f, 0.0f});
        auto transMat = glm::translate(Mat4{1.0f}, Vec3f{0.0f, -1.0f, 0.0f});
        auto mat = transMat * rotMat * scaleMat;
        quadRenderer->setTransform(mat);
    }

    auto width = _swapchain->width();
    auto height = _swapchain->height();
    scene::PerspectiveFrustum frustum{45.0f, width / (float)height, 0.1f, 50.0f};
    _cam = std::make_shared<scene::Camera>(frustum);
    auto& eye = _cam->eye();
    eye.setPosition(0.0, 0.0f, 4.0);
    eye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    eye.update();

    scene::OrthoFrustum shadowFrustum{-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 20.0f};
    _shadowCam = std::make_shared<scene::Camera>(shadowFrustum);
    auto& shadowEye = _shadowCam->eye();
    shadowEye.setPosition(5.0, 5.0, 5.0);
    shadowEye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
    shadowEye.update();

    auto& resourceGraph = _ppl->resourceGraph();
    if (!resourceGraph.contains(_forwardRT)) {
        resourceGraph.import(_forwardRT, _swapchain);
    }
    if (!resourceGraph.contains(_forwardDS)) {
        resourceGraph.addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT | rhi::ImageUsage::SAMPLED, width, height, rhi::Format::D24_UNORM_S8_UINT);
    }
    if (!resourceGraph.contains(_shadowMapRT)) {
        resourceGraph.addImage(_shadowMapRT, rhi::ImageUsage::COLOR_ATTACHMENT | rhi::ImageUsage::SAMPLED, shadowMapWidth, shadowMapHeight, rhi::Format::R32_SFLOAT);
    }
    if (!resourceGraph.contains(_shadowMapDS)) {
        resourceGraph.addImage(_shadowMapDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, shadowMapWidth, shadowMapHeight, rhi::Format::D24_UNORM_S8_UINT);
    }
    if (!resourceGraph.contains(_camBuffer)) {
        resourceGraph.addBuffer(_camBuffer, 128, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
        resourceGraph.addBuffer(_camPose, 12, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
        resourceGraph.addBuffer(_light, 32, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
        resourceGraph.addBuffer(_shadowVPBuffer, 136, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
    }
    if (!resourceGraph.contains(_shadowSampler)) {
        rhi::SamplerInfo info{};
        info.magFilter = rhi::Filter::NEAREST;
        info.minFilter = rhi::Filter::NEAREST;
        info.mipmapMode = rhi::MipmapMode::NEAREST;
        info.addressModeU = rhi::SamplerAddressMode::CLAMP_TO_BORDER;
        info.addressModeV = rhi::SamplerAddressMode::CLAMP_TO_BORDER;
        info.addressModeW = rhi::SamplerAddressMode::CLAMP_TO_BORDER;
        resourceGraph.addSampler(_shadowSampler, info);
    }
    if (!resourceGraph.contains(_sssInfo)) {
        resourceGraph.addBuffer(_sssInfo, 24, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
        resourceGraph.addBuffer(_waveOffsetBuffer, 8, graph::BufferUsage::UNIFORM | graph::BufferUsage::TRANSFER_DST);
    }
    if (!resourceGraph.contains(_sssOuput)) {
        resourceGraph.addImage(_sssOuput, rhi::ImageUsage::STORAGE | rhi::ImageUsage::SAMPLED, width, height, rhi::Format::R32_SFLOAT);
    }
}

} // namespace raum::sample
