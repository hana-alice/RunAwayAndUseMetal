#pragma once
#include <ranges>
#include "BuiltinRes.h"
#include "Camera.h"
#include "Director.h"
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
#include "renderer/feature/VirtualTexture.h"
#include "stb_image.h"
namespace raum::sample {
class VirtualTextureSample : public SampleBase {
public:
    explicit VirtualTextureSample(framework::Director* director) : _ppl(director->pipeline()), _director(director) {}

    void init() override {
        // load scene from gltf
        const auto& resourcePath = utils::resourceDirectory();
        auto& sceneGraph = _ppl->sceneGraph();
        auto device = _ppl->device();
        auto swapchain = _ppl->swapchain();

        auto& shaderGraph = _ppl->shaderGraph();
        //shaderGraph.compile("sparse");

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

        _vt = std::make_shared<render::VirtualTexture>(imageAsset.data, imageAsset.width, imageAsset.height, device);
        _vt->setMiptail(imageAssetMip6.data, 6);
        _vt->setMiptail(imageAssetMip7.data, 7);
        _vt->setMiptail(imageAssetMip8.data, 8);
        _vt->setMiptail(imageAssetMip9.data, 9);

        auto shaderRes = shaderGraph.layout("asset/layout/sparse");
        scene::SlotMap binds;
        for (const auto& [name, desc] : shaderRes.bindings) {
            if (desc.rate == graph::Rate::PER_BATCH) {
                binds.emplace(name, desc.binding);
            }
        }
        //        _vt->setSparseImageSlot("mainTexture");
        //        _vt->setSparseSamplerSlot("mainSampler");
        //        _vt->setAccessCounterSlot("accessCounter");
        //        _vt->setMetaInfoSlot("PageExtent");
        //        _vt->initLocalBind(binds,
        //                           shaderRes.descriptorLayouts.at(static_cast<uint32_t>(graph::Rate::PER_INSTANCE)));



        _cmdPool = rhi::CommandPoolPtr(device->createCoomandPool({}));
        auto* q = device->getQueue({rhi::QueueType::GRAPHICS});
        auto cmd = rhi::CommandBufferPtr(_cmdPool->makeCommandBuffer({}));
        cmd->enqueue(q);
        cmd->begin({});
        auto* sparseQ = device->getQueue({rhi::QueueType::SPARSE});
        auto* sparseSem = sparseQ->getSignal();
        _vt->prepare(cmd);
        cmd->commit();

        q->addWait(sparseSem);
        q->submit(false);

        cmd->onComplete([cmd]() mutable { cmd.reset(); });

        auto& quad = asset::BuiltinRes::quad();
        graph::ModelNode& quadNode = sceneGraph.addModel("quad");
        quadNode.model = quad.model();

        scene::MaterialTemplatePtr matTemplate = std::make_shared<scene::MaterialTemplate>("asset/layout/sparse");
        auto sparseMat = matTemplate->instantiate("asset/layout/sparse", scene::MaterialType::CUSTOM);
        sparseMat->initBindGroup(binds, shaderRes.descriptorLayouts.at(static_cast<uint32_t>(graph::Rate::PER_BATCH)), device);
        sparseMat->set("mainTexture", scene::Texture{
                                          _vt->sparseImage(),
                                          _vt->sparseView()});
        rhi::SamplerInfo samplerInfo {
            .magFilter = rhi::Filter::LINEAR,
            .minFilter = rhi::Filter::LINEAR,
            .minLod = 0,
            .maxLod = 9,
        };
        sparseMat->set("mainSampler", scene::Sampler{samplerInfo});
        sparseMat->set("PageExtent", scene::Buffer{_vt->metaInfoBuffer()});
        sparseMat->set("accessCounter", scene::Buffer{_vt->accessCounterBuffer()});
        sparseMat->update();

        _preRenderTask = framework::RenderTask{
            [this](std::chrono::milliseconds sec, rhi::CommandBufferPtr cmd, rhi::DevicePtr device){
                auto* sparseQ = device->getQueue({{rhi::QueueType::SPARSE}});
                auto* grfxQ = device->getQueue({{rhi::QueueType::GRAPHICS}});
                if(_lastGraphicsSem) {
                    //sparseQ->addWait(_lastGraphicsSem);
                }
                if (_lastSparseSem) {

                }
                _lastGraphicsSem = grfxQ->getSignal();
                _lastSparseSem = sparseQ->getSignal();
                _vt->resetAccessCounter(cmd);
                _vt->update(cmd);
                auto newUpdate = _vt->hasRemainedTask();
                if (newUpdate) {
                    grfxQ->addWait(_lastSparseSem);
                }
            }
        };
        _director->addPreRenderTask(&_preRenderTask);

        _postRenderTask = framework::RenderTask{
            [this](std::chrono::milliseconds sec, rhi::CommandBufferPtr cmd, rhi::DevicePtr device){
                _vt->invalidateFeedback(cmd);
            }
        };
        _director->addPostRenderTask(&_postRenderTask);

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

    ~VirtualTextureSample() {
        _keyListener.remove();
        _mouseListener.remove();
    }

    void show() override {
        auto device = _ppl->device();
        auto swapchain = _ppl->swapchain();

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
    framework::Director* _director;

    render::VirtualTexturePtr _vt;

    std::shared_ptr<scene::Camera> _cam;
    std::shared_ptr<scene::Scene> _scene;

    const std::string _forwardRT = "forwardRT";
    const std::string _forwardDS = "forwardDS";
    const std::string _mvp = "mvp";

    const std::string _name = "VirtualTexture";

    framework::EventListener<framework::KeyboardEventTag> _keyListener;
    framework::EventListener<framework::MouseEventTag> _mouseListener;
    //    framework::EventListener<framework::ResizeEventTag> _resizeListener;

    framework::RenderTask _preRenderTask;
    framework::RenderTask _postRenderTask;

    rhi::RHISemaphore* _lastGraphicsSem{nullptr};
    rhi::RHISemaphore* _lastSparseSem{nullptr};

    rhi::CommandPoolPtr _cmdPool;
};

} // namespace raum::sample