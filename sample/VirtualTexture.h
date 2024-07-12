#pragma once
#include "BuiltinRes.h"
#include "Camera.h"
#include "GraphScheduler.h"
#include "ImageLoader.h"
#include "KeyboardEvent.h"
#include "Mesh.h"
#include "MouseEvent.h"
#include "PBRMaterial.h"
#include "Pipeline.h"
#include "RHIBlitEncoder.h"
#include "RHICommandBuffer.h"
#include "RHICommandPool.h"
#include "RHIDevice.h"
#include "RHISparseImage.h"
#include "SceneSerializer.h"
#include "Serialization.h"
#include "WindowEvent.h"
#include "common.h"
#include "core/utils/utils.h"
#include "math.h"
#include "stb_image.h"
namespace raum::sample {
class VirtualTexture : public SampleBase {
public:
    explicit VirtualTexture(graph::PipelinePtr ppl) : _ppl(ppl) {}

    void init() override {
        // load scene from gltf
        const auto& resourcePath = utils::resourceDirectory();
        auto& sceneGraph = _ppl->sceneGraph();
        auto device = _ppl->device();
        auto swapchain = _ppl->swapchain();

        auto textureFile = resourcePath / "images" / "8k_earth_daymap.jpg";

        auto textureMip6 = resourcePath / "images" / "8k_earth_daymap_view_mip6_128x64.bmp";
        auto textureMip7 = resourcePath / "images" / "8k_earth_daymap_view_mip7_64x32.bmp";
        auto textureMip8 = resourcePath / "images" / "8k_earth_daymap_view_mip8_32x16.bmp";
        auto textureMip9 = resourcePath / "images" / "8k_earth_daymap_view_mip9_16x8.bmp";

        asset::ImageLoader loader;
        const auto& imageAsset = loader.load(textureFile.string());

        const auto& imageAssetMip6 = loader.load(textureMip6.string());
        const auto& imageAssetMip7 = loader.load(textureMip7.string());
        const auto& imageAssetMip8 = loader.load(textureMip8.string());
        const auto& imageAssetMip9 = loader.load(textureMip9.string());

        rhi::SparseImageInfo info{
            .data = imageAsset.data,
            .width = static_cast<uint32_t>(imageAsset.width),
            .height = static_cast<uint32_t>(imageAsset.height),
            .format = rhi::Format::RGBA8_UNORM,
        };
        _sparseImage = rhi::SparseImagePtr(device->createSparseImage(info));

        _cmdPool = rhi::CommandPoolPtr(device->createCoomandPool({}));

        const auto& granularity = _sparseImage->granularity();

        std::array<uint32_t, 4> pageExt = {
            (imageAsset.width + granularity.x - 1) / granularity.x,
            (imageAsset.height + granularity.y - 1) / granularity.y,
            static_cast<uint32_t>(imageAsset.width),
            static_cast<uint32_t>(imageAsset.height),
        };
        rhi::BufferSourceInfo pageExtentInfo{
            .size = 4 * sizeof(uint32_t),
            .data = pageExt.data(),
        };
        auto pageExtBuffer = rhi::BufferPtr(device->createBuffer(pageExtentInfo));

        rhi::BufferInfo accessBufferInfo{
            .memUsage = rhi::MemoryUsage::HOST_VISIBLE,
            .bufferUsage = rhi::BufferUsage::STORAGE | rhi::BufferUsage::TRANSFER_DST,
            .size = static_cast<uint32_t>(pageExt[0] * pageExt[1] * sizeof(uint32_t)),
        };
        _accessBuffer = rhi::BufferPtr(device->createBuffer(accessBufferInfo));

        auto* q = device->getQueue({rhi::QueueType::GRAPHICS});
        auto cmd = rhi::CommandBufferPtr(_cmdPool->makeCommandBuffer({}));
        cmd->enqueue(q);
        cmd->begin({});
        _sparseImage->setMiptail(imageAssetMip6.data, 6);
        _sparseImage->setMiptail(imageAssetMip7.data, 7);
        _sparseImage->setMiptail(imageAssetMip8.data, 8);
        _sparseImage->setMiptail(imageAssetMip9.data, 9);

        auto blit = rhi::BlitEncoderPtr(cmd->makeBlitEncoder());
        blit->fillBuffer(_accessBuffer.get(), 0, accessBufferInfo.size, 256);
        auto* sparseQ = device->getQueue({rhi::QueueType::SPARSE});
        auto* sparseSem = sparseQ->getSignal();
        _sparseImage->bind();
        _sparseImage->prepare(cmd.get());
        cmd->commit();

        q->addWait(sparseSem);
        q->submit(false);

        cmd->onComplete([cmd]() mutable { cmd.reset(); });

        rhi::SparseImageViewInfo viewInfo{
            .image = _sparseImage.get(),
            .range = {
                .sliceCount = 1,
                .mipCount = 10,
            },
            .format = rhi::Format::RGBA8_UNORM,
        };
        auto sparseView = rhi::ImageViewPtr(device->createImageView(viewInfo));

        auto& quad = asset::BuiltinRes::quad();
        graph::ModelNode& quadNode = sceneGraph.addModel("quad");
        quadNode.model = quad.model();

        scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/sparse");
        auto sparseMat = matTemplate->instantiate("asset/layout/sparse", scene::MaterialType::CUSTOM);
        sparseMat->set("mainTexture", {_sparseImage, sparseView});

        scene::Sampler sampler{
            rhi::SamplerInfo{
                .magFilter = rhi::Filter::LINEAR,
                .minFilter = rhi::Filter::LINEAR,
                .minLod = 0,
                .maxLod = 9,

            },
        };
        sparseMat->set("mainSampler", sampler);
        sparseMat->set("PageExtent", {pageExtBuffer});
        sparseMat->set("accessCounter", {_accessBuffer});

        scene::TechniquePtr sparseTech = std::make_shared<scene::Technique>(sparseMat, "default");
        sparseTech->setPrimitiveType(rhi::PrimitiveType::TRIANGLE_LIST);
        auto& ds = sparseTech->depthStencilInfo();
        ds.depthTestEnable = true;
        ds.depthWriteEnable = false;
        ds.depthCompareOp = rhi::CompareOp::LESS_OR_EQUAL;
        auto& bs = sparseTech->blendInfo();
        bs.attachmentBlends.emplace_back();

        auto& meshRenderer = quadNode.model->meshRenderers().front();
        meshRenderer->removeTechnique(0);
        meshRenderer->addTechnique(sparseTech);

        _cmdBuffers.resize(rhi::FRAMES_IN_FLIGHT);
        _sparseCmds.resize(rhi::FRAMES_IN_FLIGHT);
        for (auto& cmd : _cmdBuffers) {
            cmd = rhi::CommandBufferPtr(_cmdPool->makeCommandBuffer({}));
        }
        // for (auto& cmd : _sparseCmds) {
        //     cmd = rhi::CommandBufferPtr(_sparseCmdPool->makeCommandBuffer({}));
        // }

        auto width = swapchain->width();
        auto height = swapchain->height();
        scene::Frustum frustum{45.0f, width / (float)height, 0.1f, 1000.0f};
        _cam = std::make_shared<scene::Camera>(frustum, scene::Projection::PERSPECTIVE);
        auto& eye = _cam->eye();
        eye.setPosition(0.0, 0.0f, 4.0);
        eye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        eye.update();

        auto& resourceGraph = _ppl->resourceGraph();
        if (!resourceGraph.contains(_forwardRT)) {
            resourceGraph.import(_forwardRT, swapchain);
        }
        if (!resourceGraph.contains(_forwardDS)) {
            resourceGraph.addImage(_forwardDS, rhi::ImageUsage::DEPTH_STENCIL_ATTACHMENT, width, height, rhi::Format::D24_UNORM_S8_UINT);
        }
        if (!resourceGraph.contains(_mvp)) {
            resourceGraph.addBuffer(_mvp, 192, graph::BufferUsage ::UNIFORM | graph::BufferUsage ::TRANSFER_DST);
        }

        // listeners
        auto keyHandler = [&](framework::Keyboard key, framework::KeyboardType type) {
            if (key != framework::Keyboard::OTHER && type == framework::KeyboardType::PRESS) {
                auto front = _cam->eye().forward();
                front = glm::normalize(front) * 0.1f;
                auto right = glm::cross(front, _cam->eye().up());
                right = glm::normalize(right) * 0.1f;
                if (key == framework::Keyboard::W) {
                    _cam->eye().translate(front);
                } else if (key == framework::Keyboard::S) {
                    _cam->eye().translate(-front);
                } else if (key == framework::Keyboard::A) {
                    _cam->eye().translate(-right);
                } else if (key == framework::Keyboard::D) {
                    _cam->eye().translate(right);
                }
                _cam->eye().update();
            }
        };
        _keyListener.add(keyHandler);

        auto mouseHandler = [&, width, height](int32_t x, int32_t y, framework::MouseButton btn, framework::ButtonStatus status) {
            static bool firstPress{true};
            static bool pressed{false};
            static int32_t lastX = x;
            static int32_t lastY = y;
            if (status == framework::ButtonStatus::RELEASE) {
                firstPress = true;
                pressed = false;
            } else if (status == framework::ButtonStatus::PRESS && btn != framework::MouseButton::OTHER) {
                pressed = true;
            }
            if (pressed) {
                if (firstPress) {
                    firstPress = false;
                    lastX = x;
                    lastY = y;
                } else {
                    static float curDeg = 0.0f;
                    auto deltaX = x - lastX;
                    curDeg += deltaX * 0.1f;
                    auto radius = 4.0f;

                    auto curRad = curDeg / 180.0f * 3.141593f;
                    auto zpos = radius * cos(-curRad);
                    auto xpos = radius * sin(-curRad);

                    auto& eye = _cam->eye();
                    eye.setPosition(xpos, 0.0f, zpos);
                    eye.lookAt({0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
                    eye.update();
                    lastX = x;
                    lastY = y;

                    if (curRad > 6.283186f) {
                        curRad -= 6.283186f;
                    }
                }
            }
        };
        //_mouseListener.add(mouseHandler);
    }

    ~VirtualTexture() {
        _keyListener.remove();
        _mouseListener.remove();
    }

    void show() override {
        auto device = _ppl->device();
        auto swapchain = _ppl->swapchain();
        auto* grfxQ = device->getQueue({rhi::QueueType::GRAPHICS});
        auto* sparseQ = device->getQueue({rhi::QueueType::SPARSE});

        static bool firstFrame{true};
        if (!firstFrame) {
            auto* sparseSem = sparseQ->getSignal();
            auto cmd = _cmdBuffers[swapchain->imageIndex()];
            cmd->reset();
            cmd->enqueue(grfxQ);
            cmd->begin({});
            _sparseImage->analyze(_accessBuffer.get(), cmd.get());
            _sparseImage->bind();
            _sparseImage->update(cmd.get());
            auto blit = rhi::BlitEncoderPtr(cmd->makeBlitEncoder());
            blit->fillBuffer(_accessBuffer.get(), 0, _accessBuffer->info().size, 256);
            cmd->commit();
            grfxQ->addWait(sparseSem);
        }
        firstFrame = false;

        auto& renderGraph = _ppl->renderGraph();
        auto uploadPass = renderGraph.addCopyPass("cambufferUpdate");

        _ppl->resourceGraph().updateImage("forwardDS", swapchain->width(), swapchain->height());

        auto& eye = _cam->eye();
        Mat4 modelMat = Mat4(1.0f);
        uploadPass.uploadBuffer(&modelMat[0], 64, _mvp, 0);
        auto viewMat = eye.inverseAttitide();
        uploadPass.uploadBuffer(&viewMat[0], 64, _mvp, 64);
        const auto& projMat = eye.projection();
        uploadPass.uploadBuffer(&projMat[0], 64, _mvp, 128);

        auto renderPass = renderGraph.addRenderPass("forward");
        renderPass.addColor(_forwardRT, graph::LoadOp::CLEAR, graph::StoreOp::STORE, {0.3, 0.3, 0.3, 1.0})
            .addDepthStencil(_forwardDS, graph::LoadOp::CLEAR, graph::StoreOp::STORE, graph::LoadOp::CLEAR, graph::StoreOp::STORE, 1.0, 0);
        auto queue = renderPass.addQueue("default");

        auto width = swapchain->width();
        auto height = swapchain->height();
        queue.setViewport(0, 0, width, height, 0.0f, 1.0f)
            .addCamera(_cam.get())
            .addUniformBuffer(_mvp, "Mat");
    }

    void hide() override {
        _ppl->sceneGraph().disable("quad");
    }

    const std::string& name() override {
        return _name;
    }

private:
    graph::PipelinePtr _ppl;

    std::shared_ptr<scene::Camera> _cam;
    std::shared_ptr<scene::Scene> _scene;

    const std::string _forwardRT = "forwardRT";
    const std::string _forwardDS = "forwardDS";
    const std::string _mvp = "mvp";

    const std::string _name = "VirtualTexture";

    rhi::SparseImagePtr _sparseImage;

    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseEventTag> _mouseListener;
    //    framework::EventListener<framework::ResizeEventTag> _resizeListener;

    std::vector<rhi::CommandBufferPtr> _cmdBuffers;
    std::vector<rhi::CommandBufferPtr> _sparseCmds;
    rhi::CommandPoolPtr _cmdPool;
    rhi::CommandPoolPtr _sparseCmdPool;
    rhi::BufferPtr _accessBuffer;
};

} // namespace raum::sample