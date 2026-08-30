#pragma once

#include "BuiltinRes.h"
#include "CameraSample.h"
#include "Director.h"
#include "ImageLoader.h"
#include "PBRMaterial.h"
#include "Pipeline.h"
#include "RHICommandBuffer.h"
#include "RHICommandPool.h"
#include "RHIDevice.h"
#include "core/utils/utils.h"
#include "renderer/feature/VirtualTexture.h"

namespace raum::sample {

class VirtualTextureSample final : public CameraSample {
public:
    explicit VirtualTextureSample(framework::Director* director)
    : CameraSample(director, "VirtualTexture") {}

    ~VirtualTextureSample() override {
        unregisterRenderTasks();
    }

private:
    void onInitialize() override {
        const auto& resourcePath = utils::resourceDirectory();
        auto& shaderGraph = pipeline().shaderGraph();

        asset::ImageLoader loader;
        const auto& imageAsset = loader.load((resourcePath / "images" / "8k_earth_daymap.jpg").string());
        const auto& imageAssetMip6 = loader.load((resourcePath / "images" / "8k_earth_daymap_view_mip6_128x64.bmp").string());
        const auto& imageAssetMip7 = loader.load((resourcePath / "images" / "8k_earth_daymap_view_mip7_64x32.bmp").string());
        const auto& imageAssetMip8 = loader.load((resourcePath / "images" / "8k_earth_daymap_view_mip8_32x16.bmp").string());
        const auto& imageAssetMip9 = loader.load((resourcePath / "images" / "8k_earth_daymap_view_mip9_16x8.bmp").string());

        _virtualTexture = std::make_shared<render::VirtualTexture>(
            imageAsset.data,
            imageAsset.width,
            imageAsset.height,
            device());
        _virtualTexture->setMiptail(imageAssetMip6.data, 6);
        _virtualTexture->setMiptail(imageAssetMip7.data, 7);
        _virtualTexture->setMiptail(imageAssetMip8.data, 8);
        _virtualTexture->setMiptail(imageAssetMip9.data, 9);

        const auto shaderResources = shaderGraph.layout("asset/layout/sparse");
        scene::SlotMap bindings;
        for (const auto& [bindingName, description] : shaderResources.bindings) {
            if (description.rate == graph::Rate::PER_BATCH) {
                bindings.emplace(bindingName, description.binding);
            }
        }

        prepareVirtualTexture();

        const auto& quadName = resource("quad");
        auto& quadNode = sceneGraph().addModel(quadName);
        quadNode.model = asset::BuiltinRes::quad().model();
        trackSceneNode(quadName);

        auto materialTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/sparse");
        auto sparseMaterial = materialTemplate->instantiate("asset/layout/sparse", scene::MaterialType::CUSTOM);
        sparseMaterial->initBindGroup(
            bindings,
            shaderResources.descriptorLayouts.at(static_cast<uint32_t>(graph::Rate::PER_BATCH)),
            device());
        sparseMaterial->set("mainTexture", scene::Texture{
                                               _virtualTexture->sparseImage(),
                                               _virtualTexture->sparseView(),
                                           });
        sparseMaterial->set("mainSampler", scene::Sampler{rhi::SamplerInfo{
                                               .magFilter = rhi::Filter::LINEAR,
                                               .minFilter = rhi::Filter::LINEAR,
                                               .minLod = 0,
                                               .maxLod = 9,
                                           }});
        sparseMaterial->set("PageExtent", scene::Buffer{_virtualTexture->metaInfoBuffer()});
        sparseMaterial->set("accessCounter", scene::Buffer{_virtualTexture->accessCounterBuffer()});
        sparseMaterial->update();

        auto sparseTechnique = std::make_shared<scene::Technique>(sparseMaterial, "default");
        sparseTechnique->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
        auto& depthStencil = sparseTechnique->depthStencilInfo();
        depthStencil.depthTestEnable = true;
        depthStencil.depthWriteEnable = false;
        depthStencil.depthCompareOp = rhi::CompareOp::LESS_OR_EQUAL;
        sparseTechnique->blendInfo().attachmentBlends.emplace_back();

        auto& meshRenderer = quadNode.model->meshRenderers().front();
        meshRenderer->removeTechnique(0);
        meshRenderer->addTechnique(sparseTechnique);

        createRenderTasks();
        createPerspectiveCamera({
            .verticalFov = utils::Degree{45.0f},
            .nearPlane = 0.1f,
            .farPlane = 1000.0f,
            .position = {0.0f, 0.0f, 4.0f},
            .yawDegrees = 180.0f,
        });
        enableFlyCamera({.moveSpeed = 0.1f});

        ensureSwapchain("present");
        ensureViewportImage(
            "forwardDepth",
            rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT,
            rhi::Format::D24_UNORM_S8_UINT);
        ensureBuffer(
            "mvp",
            3 * sizeof(Mat4),
            rhi::BufferUsage::UNIFORM | rhi::BufferUsage::TRANSFER_DST);
    }

    void onActivated() override {
        if (_renderTasksRegistered) {
            return;
        }
        director().addPreRenderTask(&_preRenderTask);
        director().addPostRenderTask(&_postRenderTask);
        _renderTasksRegistered = true;
    }

    void onDeactivated() override {
        unregisterRenderTasks();
    }

    void onRender() override {
        auto& renderGraph = pipeline().renderGraph();
        resizeViewportResources();

        auto uploadPass = renderGraph.addCopyPass("cameraBufferUpdate");
        const Mat4 model{1.0f};
        uploadPass.uploadBuffer(&model[0], sizeof(Mat4), resource("mvp"), 0);
        uploadPass.uploadBuffer(&camera().eye().attitude()[0], sizeof(Mat4), resource("mvp"), sizeof(Mat4));
        uploadPass.uploadBuffer(&camera().eye().projection()[0], sizeof(Mat4), resource("mvp"), 2 * sizeof(Mat4));

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
            .addUniformBuffer(resource("mvp"), "Mat");
    }

    void prepareVirtualTexture() {
        auto graphicsQueue = device()->getQueue({rhi::QueueType::GRAPHICS});
        _commandPool = rhi::CommandPoolPtr(device()->createCoomandPool({graphicsQueue->index()}));
        auto commandBuffer = rhi::CommandBufferPtr(_commandPool->makeCommandBuffer({}));
        commandBuffer->enqueue(graphicsQueue);
        commandBuffer->begin({});

        auto sparseQueue = device()->getQueue({rhi::QueueType::SPARSE});
        _lastSparseSemaphore = device()->createSemaphore();
        sparseQueue->addSignal(_lastSparseSemaphore);
        _lastGraphicsSemaphore = device()->createSemaphore();
        _virtualTexture->prepare(commandBuffer);
        commandBuffer->commit();

        graphicsQueue->addWait(_lastSparseSemaphore);
        graphicsQueue->submit(false);
        commandBuffer->onComplete([commandBuffer]() mutable { commandBuffer.reset(); });
    }

    void createRenderTasks() {
        _preRenderTask = framework::RenderTask{
            [this](std::chrono::milliseconds, rhi::CommandBufferPtr commandBuffer, rhi::DevicePtr taskDevice) {
                auto sparseQueue = taskDevice->getQueue({rhi::QueueType::SPARSE});
                auto graphicsQueue = taskDevice->getQueue({rhi::QueueType::GRAPHICS});
                _virtualTexture->resetAccessCounter(commandBuffer);
                _virtualTexture->analyze(commandBuffer);
                if (_virtualTexture->hasRemainedTask()) {
                    sparseQueue->addWait(_lastGraphicsSemaphore);
                    sparseQueue->addSignal(_lastSparseSemaphore);
                    graphicsQueue->addWait(_lastSparseSemaphore);
                    graphicsQueue->addSignal(_lastGraphicsSemaphore);
                }
                _virtualTexture->update(commandBuffer);
            }};
        _postRenderTask = framework::RenderTask{
            [this](std::chrono::milliseconds, rhi::CommandBufferPtr commandBuffer, rhi::DevicePtr) {
                _virtualTexture->invalidateFeedback(commandBuffer);
            }};
    }

    void unregisterRenderTasks() {
        if (!_renderTasksRegistered) {
            return;
        }
        director().removePreRenderTask(&_preRenderTask);
        director().removePostRenderTask(&_postRenderTask);
        _renderTasksRegistered = false;
    }

    render::VirtualTexturePtr _virtualTexture;
    framework::RenderTask _preRenderTask;
    framework::RenderTask _postRenderTask;
    rhi::RHISemaphore* _lastGraphicsSemaphore{nullptr};
    rhi::RHISemaphore* _lastSparseSemaphore{nullptr};
    rhi::CommandPoolPtr _commandPool;
    bool _renderTasksRegistered{false};
};

} // namespace raum::sample
