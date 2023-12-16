#pragma once

#include <cstdlib>
#include <iostream>
#include "Model.hpp"
#include "Triangle.hpp"
#include "RHIManager.h"
#include "RHICommandBuffer.h"
#include "RHIImage.h"
#include "RHIImageView.h"
#include "RHIRenderEncoder.h"
#include "RHIRenderPass.h"
#include "RHIFrameBuffer.h"
#include <map>
#include <string>
#include "RHIBuffer.h"
#include "RHIPipelineLayout.h"
#include "RHIGraphicsPipeline.h"
#include "RHIShader.h"
namespace raum {
using platform::NativeWindow;
using namespace rhi;

constexpr uint32_t width = 1080u;
constexpr uint32_t height = 720u;

const std::string vertStr = R"(
#version 450 core

layout(location = 0) in vec3 aPos;

void main () {
    gl_Position = vec4(aPos, 1.0f);
}
)";

const std::string fragStr = R"(
#version 450 core

layout(location = 0) out vec4 FragColor;
void main () {
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)";

class Sample {
public:
    Sample() {
        _window = std::make_shared<NativeWindow>(width, height);
        _device = loadRHI(API::VULKAN);
        _swaphchain = _device->createSwapchain(SwapchainInfo{width, height, SyncType::RELAX, _window->handle()});
        _window->registerPollEvents(std::bind(&Sample::render, this));
        _queue = _device->getQueue(QueueInfo{QueueType::GRAPHICS});
        
        RenderPassInfo rpInfo{};
        AttachmentInfo attachmentInfo{};
        attachmentInfo.loadOp = LoadOp::CLEAR;
        attachmentInfo.storeOp = StoreOp::STORE;
        attachmentInfo.format = _swaphchain->swapchainImageView()->viewInfo().format;
        attachmentInfo.initialLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
        attachmentInfo.finalLayout = ImageLayout::PRESENT;
        rpInfo.attachments.emplace_back(attachmentInfo);
        _renderpass = _device->createRenderPass(rpInfo);

        float vertices[] = {
            -0.5f, -0.5f, 0.5f,
            0.5f, -0.5f, 0.5f,
            0.0f, 0.5f, 0.5f
        };
        BufferSourceInfo vertInfo{};
        vertInfo.bufferUsage = BufferUsage::VERTEX;
        vertInfo.memUsage = MemoryUsage::DEVICE_ONLY;
        vertInfo.size = sizeof(vertices);
        vertInfo.data = vertices;
        _vertexBuffer = _device->createBuffer(vertInfo);

        uint16_t indices[] = {
            0, 1, 2,
        };
        BufferSourceInfo indexInfo{};
        indexInfo.bufferUsage = BufferUsage::INDEX;
        indexInfo.memUsage = MemoryUsage::DEVICE_ONLY;
        indexInfo.size = sizeof(indices);
        indexInfo.data = indices;
        _indexBuffer = _device->createBuffer(indexInfo);

        ShaderSourceInfo vertShaderInfo{};
        vertShaderInfo.sourcePath = "/vertStr";
        vertShaderInfo.stage.source = vertStr;
        vertShaderInfo.stage.stage = ShaderStage::VERTEX;
        _vertShader = _device->createShader(vertShaderInfo);

        ShaderSourceInfo fragShaderInfo{};
        fragShaderInfo.sourcePath = "/fragStr";
        fragShaderInfo.stage.source = fragStr;
        fragShaderInfo.stage.stage = ShaderStage::FRAGMENT;
        _fragShader = _device->createShader(fragShaderInfo);

        PipelineLayoutInfo layoutInfo{};
        _pipelineLayout = _device->createPipelineLayout(layoutInfo);

        VertexLayout vertLayout{};
        VertexAttribute attribute{0 , 0, Format::R32G32B32_SFLOAT, 0};
        vertLayout.vertexAttrs.emplace_back(attribute);
        VertexBufferAttribute vertBufferAttribute{0, sizeof(vertices) / 3, InputRate::PER_VERTEX};
        vertLayout.vertexBufferAttrs.emplace_back(vertBufferAttribute);

        BlendInfo blendInfo{};
        blendInfo.attachmentBlends.emplace_back();
        MultisamplingInfo msInfo{};

        GraphicsPipelineInfo pipelineInfo{};
        pipelineInfo.pipelineLayout = _pipelineLayout;
        pipelineInfo.renderPass = _renderpass;
        pipelineInfo.shaders.emplace_back(_vertShader);
        pipelineInfo.shaders.emplace_back(_fragShader);
        pipelineInfo.vertexLayout = vertLayout;
        pipelineInfo.colorBlendInfo = blendInfo;
        pipelineInfo.multisamplingInfo = msInfo;
        _pipeline = _device->createGraphicsPipeline(pipelineInfo);

    }

    ~Sample() {}

    void show() {
        _window->mainLoop();
    }

private:
    void render() {
        _swaphchain->aquire();
        auto* scImageView = _swaphchain->swapchainImageView();

        if (_commandBuffers.find(scImageView) == _commandBuffers.end()) {
            _commandBuffers[scImageView] = _queue->makeCommandBuffer({});
        }
        auto* commandbuffer = _commandBuffers[scImageView];

        commandbuffer->reset();

        commandbuffer->begin({});

        ImageBarrierInfo imageBarrierInfo{};
        imageBarrierInfo.image = _swaphchain->swapchainImageView()->image();
        imageBarrierInfo.oldLayout = ImageLayout::UNDEFINED;
        imageBarrierInfo.newLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
        imageBarrierInfo.srcAccessFlag = AccessFlags::COLOR_ATTACHMENT_WRITE;
        imageBarrierInfo.dstAccessFlag = AccessFlags::NONE;
        imageBarrierInfo.srcQueueIndex = _queue->index();
        imageBarrierInfo.dstQueueIndex = _queue->index();
        imageBarrierInfo.srcStage = PipelineStage::COLOR_ATTACHMENT_OUTPUT;
        imageBarrierInfo.dstStage = PipelineStage::BOTTOM_OF_PIPE;
        imageBarrierInfo.range = _swaphchain->swapchainImageView()->viewInfo().range;

        commandbuffer->appendImageBarrier(imageBarrierInfo);
        commandbuffer->applyBarrier(DependencyFlags::BY_REGION);

        if (_framebuffers.find(scImageView) == _framebuffers.end()) {
            FrameBufferInfo fbInfo{};
            fbInfo.width = width;
            fbInfo.height = height;
            fbInfo.layers = 1;
            fbInfo.renderPass = _renderpass;
            fbInfo.images.emplace_back(scImageView);
            _framebuffers[scImageView] = _device->createFrameBuffer(fbInfo);
        }
        auto* frameBuffer = _framebuffers.at(scImageView);

        std::shared_ptr<RHIRenderEncoder> encoder(commandbuffer->makeRenderEncoder());

        RenderPassBeginInfo beginInfo{};
        beginInfo.renderPass = _renderpass;
        beginInfo.frameBuffer = frameBuffer;
        auto* imageView = _swaphchain->swapchainImageView();
        beginInfo.renderArea = {0, 0, frameBuffer->info().width, frameBuffer->info().height};
        ClearColor clearColor{1.0, 0.0, 0.0, 1.0};
        beginInfo.clearColors = &clearColor;

        encoder->beginRenderPass(beginInfo);
        Viewport vp{0, 0, frameBuffer->info().width, frameBuffer->info().height, 0.0, 1.0};
        encoder->setViewport(vp);
        encoder->setScissor({0, 0, frameBuffer->info().width, frameBuffer->info().height});
        encoder->bindPipeline(_pipeline);
        encoder->bindVertexBuffer(_vertexBuffer, 0);
        encoder->bindIndexBuffer(_indexBuffer, 0, IndexType::HALF);
        encoder->draw(3, 1, 0, 0);
        encoder->endRenderPass();

        commandbuffer->commit();

        _queue->submit();
        _swaphchain->present();
    }
    std::shared_ptr<NativeWindow> _window;
    RHIDevice* _device;
    RHISwapchain* _swaphchain;
    RHIQueue* _queue;
    std::map<RHIImageView*, RHICommandBuffer*> _commandBuffers;
    RHIRenderPass* _renderpass;
    std::map<RHIImageView*, RHIFrameBuffer*> _framebuffers;
    RHIBuffer* _vertexBuffer;
    RHIBuffer* _indexBuffer;
    RHIShader* _vertShader;
    RHIShader* _fragShader;
    RHIGraphicsPipeline* _pipeline;
    RHIPipelineLayout* _pipelineLayout;
};

} // namespace raum

int main() {
    raum::Sample sample{};
    sample.show();
    return 0;
}