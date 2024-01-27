#pragma once

#include <map>
#include <string>
#include "Model.hpp"
#include "RHIBlitEncoder.h"
#include "RHIBuffer.h"
#include "RHICommandBuffer.h"
#include "RHIFrameBuffer.h"
#include "RHIGraphicsPipeline.h"
#include "RHIImage.h"
#include "RHIImageView.h"
#include "RHIManager.h"
#include "RHIPipelineLayout.h"
#include "RHIRenderEncoder.h"
#include "RHIRenderPass.h"
#include "RHIShader.h"
#include "common.h"
namespace raum::sample {

namespace {
const std::string triVertStr = R"(
#version 450 core

layout(location = 0) in vec3 aPos;

void main () {
    gl_Position = vec4(aPos, 1.0f);
}
)";

const std::string triFragStr = R"(
#version 450 core

layout(location = 0) out vec4 FragColor;
void main () {
    FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
}
)";

float triVertices[] = {
    -0.5f, -0.5f, 0.5f,
    0.5f, -0.5f, 0.5f,
    0.0f, 0.5f, 0.5f};

uint32_t triIndices[] = {
    0,
    1,
    2,
};

} // namespace

using namespace ::raum::rhi;

class Triangle : public raum::sample::SampleBase {
public:
    Triangle(std::shared_ptr<RHIDevice> device, std::shared_ptr<RHISwapchain> swapchain);
    ~Triangle();
    Triangle(const Triangle&) = delete;
    Triangle(Triangle&&) = delete;

    void show() override;

private:
    bool _firstFrame{true};
    std::shared_ptr<RHIDevice> _device;
    std::shared_ptr<RHISwapchain> _swaphchain;
    std::shared_ptr<RHICommandPool> _commandPool;
    std::map<RHIImageView*, std::shared_ptr<RHICommandBuffer>> _commandBuffers;
    std::shared_ptr<RHIRenderPass> _renderpass;
    std::map<RHIImageView*, std::shared_ptr<RHIFrameBuffer>> _framebuffers;
    std::shared_ptr<RHIBuffer> _vertexBuffer;
    std::shared_ptr<RHIBuffer> _indexBuffer;
    std::shared_ptr<RHIShader> _vertShader;
    std::shared_ptr<RHIShader> _fragShader;
    std::shared_ptr<RHIGraphicsPipeline> _pipeline;
    std::shared_ptr<RHIPipelineLayout> _pipelineLayout;

    RHIQueue* _queue;
};

Triangle::Triangle(std::shared_ptr<RHIDevice> device, std::shared_ptr<RHISwapchain> swapchain)
: _device(device), _swaphchain(swapchain) {
    _queue = _device->getQueue(QueueInfo{QueueType::GRAPHICS});

    CommandPoolInfo cmdPoolInfo{};
    cmdPoolInfo.queueFamilyIndex = _queue->index();
    _commandPool = std::shared_ptr<RHICommandPool>(_device->createCoomandPool(cmdPoolInfo));

    RenderPassInfo rpInfo{};
    AttachmentInfo attachmentInfo{};
    attachmentInfo.loadOp = LoadOp::CLEAR;
    attachmentInfo.storeOp = StoreOp::STORE;
    attachmentInfo.format = _swaphchain->swapchainImageView()->viewInfo().format;
    attachmentInfo.initialLayout = ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
    attachmentInfo.finalLayout = ImageLayout::PRESENT;
    rpInfo.attachments.emplace_back(attachmentInfo);
    _renderpass = std::shared_ptr<RHIRenderPass>(_device->createRenderPass(rpInfo));

    BufferInfo vertInfo{};
    vertInfo.bufferUsage = BufferUsage::VERTEX | BufferUsage::TRANSFER_DST;
    vertInfo.memUsage = MemoryUsage::DEVICE_ONLY;
    vertInfo.size = sizeof(triVertices);
    _vertexBuffer = std::shared_ptr<RHIBuffer>(_device->createBuffer(vertInfo));

    BufferInfo indexInfo{};
    indexInfo.bufferUsage = BufferUsage::INDEX | BufferUsage::TRANSFER_DST;
    indexInfo.memUsage = MemoryUsage::DEVICE_ONLY;
    indexInfo.size = sizeof(triIndices);
    _indexBuffer = std::shared_ptr<RHIBuffer>(_device->createBuffer(indexInfo));

    ShaderSourceInfo vertShaderInfo{};
    vertShaderInfo.sourcePath = "/triVertStr";
    vertShaderInfo.stage.source = triVertStr;
    vertShaderInfo.stage.stage = ShaderStage::VERTEX;
    _vertShader = std::shared_ptr<RHIShader>(_device->createShader(vertShaderInfo));

    ShaderSourceInfo fragShaderInfo{};
    fragShaderInfo.sourcePath = "/triFragStr";
    fragShaderInfo.stage.source = triFragStr;
    fragShaderInfo.stage.stage = ShaderStage::FRAGMENT;
    _fragShader = std::shared_ptr<RHIShader>(_device->createShader(fragShaderInfo));

    PipelineLayoutInfo layoutInfo{};
    _pipelineLayout = std::shared_ptr<RHIPipelineLayout>(_device->createPipelineLayout(layoutInfo));

    VertexLayout vertLayout{};
    VertexAttribute attribute{0, 0, Format::RGB32_SFLOAT, 0};
    vertLayout.vertexAttrs.emplace_back(attribute);
    VertexBufferAttribute vertBufferAttribute{0, sizeof(triVertices) / 3, InputRate::PER_VERTEX};
    vertLayout.vertexBufferAttrs.emplace_back(vertBufferAttribute);

    BlendInfo blendInfo{};
    blendInfo.attachmentBlends.emplace_back();
    MultisamplingInfo msInfo{};

    GraphicsPipelineInfo pipelineInfo{};
    pipelineInfo.pipelineLayout = _pipelineLayout.get();
    pipelineInfo.renderPass = _renderpass.get();
    pipelineInfo.shaders.emplace_back(_vertShader.get());
    pipelineInfo.shaders.emplace_back(_fragShader.get());
    pipelineInfo.vertexLayout = vertLayout;
    pipelineInfo.colorBlendInfo = blendInfo;
    pipelineInfo.multisamplingInfo = msInfo;
    _pipeline = std::shared_ptr<RHIGraphicsPipeline>(_device->createGraphicsPipeline(pipelineInfo));
}
Triangle::~Triangle() {
}

void Triangle::show() {
    _swaphchain->aquire();
    auto* scImageView = _swaphchain->swapchainImageView();

    if (_commandBuffers.find(scImageView) == _commandBuffers.end()) {
        _commandBuffers[scImageView] = std::shared_ptr<RHICommandBuffer>(_commandPool->makeCommandBuffer({}));
    }
    auto commandbuffer = _commandBuffers[scImageView];

    commandbuffer->reset();

    commandbuffer->begin({});

    if (_firstFrame) {
        std::shared_ptr<RHIBlitEncoder> blitEncoder(commandbuffer->makeBlitEncoder());
        blitEncoder->updateBuffer(_vertexBuffer.get(), 0, triVertices, sizeof(triVertices));
        blitEncoder->updateBuffer(_indexBuffer.get(), 0, triIndices, sizeof(triIndices));

        BufferBarrierInfo bufferBarrierInfo{};
        bufferBarrierInfo.buffer = _vertexBuffer.get();
        bufferBarrierInfo.srcStage = PipelineStage::TRANSFER;
        bufferBarrierInfo.dstStage = PipelineStage::VERTEX_INPUT;
        bufferBarrierInfo.srcQueueIndex = _queue->index();
        bufferBarrierInfo.dstQueueIndex = _queue->index();
        bufferBarrierInfo.size = _vertexBuffer->info().size;
        commandbuffer->appendBufferBarrier(bufferBarrierInfo);

        bufferBarrierInfo.buffer = _indexBuffer.get();
        bufferBarrierInfo.size = _indexBuffer->info().size;
        commandbuffer->appendBufferBarrier(bufferBarrierInfo);
        _firstFrame = false;
    }

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

    auto width = scImageView->viewInfo().image->info().extent.x;
    auto height = scImageView->viewInfo().image->info().extent.y;
    if (_framebuffers.find(scImageView) == _framebuffers.end()) {
        FrameBufferInfo fbInfo{};
        fbInfo.width = width;
        fbInfo.height = height;
        fbInfo.layers = 1;
        fbInfo.renderPass = _renderpass.get();
        fbInfo.images.emplace_back(scImageView);
        _framebuffers[scImageView] = std::shared_ptr<RHIFrameBuffer>(_device->createFrameBuffer(fbInfo));
    }
    auto frameBuffer = _framebuffers.at(scImageView);

    std::shared_ptr<RHIRenderEncoder> encoder(commandbuffer->makeRenderEncoder());

    RenderPassBeginInfo beginInfo{};
    beginInfo.renderPass = _renderpass.get();
    beginInfo.frameBuffer = frameBuffer.get();
    auto* imageView = _swaphchain->swapchainImageView();
    beginInfo.renderArea = {0, 0, frameBuffer->info().width, frameBuffer->info().height};
    ClearColor clearColor{1.0, 0.0, 0.0, 1.0};
    beginInfo.clearColors = &clearColor;

    encoder->beginRenderPass(beginInfo);
    Viewport vp{0, 0, frameBuffer->info().width, frameBuffer->info().height, 0.0, 1.0};
    encoder->setViewport(vp);
    encoder->setScissor({0, 0, frameBuffer->info().width, frameBuffer->info().height});
    encoder->bindPipeline(_pipeline.get());
    encoder->bindVertexBuffer(_vertexBuffer.get(), 0);
    encoder->bindIndexBuffer(_indexBuffer.get(), 0, IndexType::FULL);
    encoder->draw(3, 1, 0, 0);
    encoder->endRenderPass();

    commandbuffer->commit(_queue);

    _queue->submit();
    _swaphchain->present();
};

} // namespace raum::sample
