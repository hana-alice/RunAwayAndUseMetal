#pragma once

#include <filesystem>
#include <map>
#include <string>
#include "ImageLoader.h"
#include "Model.hpp"
#include "RHIBlitEncoder.h"
#include "RHIBuffer.h"
#include "RHICommandBuffer.h"
#include "RHIDescriptorPool.h"
#include "RHIDescriptorSet.h"
#include "RHIDescriptorSetLayout.h"
#include "RHIFrameBuffer.h"
#include "RHIGraphicsPipeline.h"
#include "RHIImage.h"
#include "RHIImageView.h"
#include "RHIManager.h"
#include "RHIPipelineLayout.h"
#include "RHIRenderEncoder.h"
#include "RHIRenderPass.h"
#include "RHIShader.h"
#include "RHIUtils.h"
#include "common.h"
#include "glm.hpp"
#include "glm/gtx/quaternion.hpp"

namespace raum::sample {

namespace {

const std::string rcubeVertStr = R"(
#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 v_uv;

layout(location = 0) out vec2 f_uv;

layout(set = 0, binding = 0) uniform Mat {
    mat4 modelMat;
    mat4 projectMat;
};

void main () {
    f_uv = v_uv;
    gl_Position =  projectMat * modelMat * vec4(aPos.xy, aPos.z, 1.0f);
}

)";
const std::string rcubeFragStr = R"(
#version 450 core

layout(location = 0) in vec2 f_uv;
layout(location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform texture2D mainTexture;
layout(set = 0, binding = 2) uniform sampler mainSampler;


void main () {
    FragColor = texture(sampler2D(mainTexture, mainSampler), f_uv);
}
)";

float rcubeVertices[] = {
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 0.0f,

    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,

    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, -0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, -0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f, 0.0f, 1.0f,

    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f,
    0.5f, 0.5f, -0.5f, 1.0f, 1.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.5f, 1.0f, 0.0f,
    -0.5f, 0.5f, 0.5f, 0.0f, 0.0f,
    -0.5f, 0.5f, -0.5f, 0.0f, 1.0f};

} // namespace

using namespace raum::rhi;
class RotatingCube : public SampleBase {
public:
    RotatingCube(std::shared_ptr<RHIDevice> device, std::shared_ptr<RHISwapchain> swapchain);
    ~RotatingCube(){};

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
    std::shared_ptr<RHIBuffer> _uniformBuffer;
    std::shared_ptr<RHIShader> _vertShader;
    std::shared_ptr<RHIShader> _fragShader;
    std::shared_ptr<RHIGraphicsPipeline> _pipeline;
    std::shared_ptr<RHIPipelineLayout> _pipelineLayout;
    std::shared_ptr<RHIDescriptorPool> _descriptorPool;
    std::shared_ptr<RHIDescriptorSet> _descriptorSet;
    std::shared_ptr<RHIDescriptorSetLayout> _descriptorSetLayout;
    std::shared_ptr<RHIImage> _image;
    std::shared_ptr<RHIImageView> _imageView;
    std::shared_ptr<RHIBuffer> _imgdataBuffer;

    RHISampler* _sampler;

    glm::mat4 _udata[2];

    RHIQueue* _queue;
};

RotatingCube::RotatingCube(std::shared_ptr<RHIDevice> device, std::shared_ptr<RHISwapchain> swapchain) : _device(device), _swaphchain(swapchain) {
    _queue = _device->getQueue(QueueInfo{QueueType::GRAPHICS});

    framework::asset::ImageLoader loader{};
    const auto& curPath = std::filesystem::current_path();
    auto filepath = curPath / "resources" / "liangfeifan.png";
    auto imgAsset = loader.load(filepath.string());

    std::cout << curPath.string().c_str() << std::endl;

    BufferSourceInfo idataInfo{};
    idataInfo.bufferUsage = BufferUsage::TRANSFER_SRC;
    idataInfo.queueAccess = {{_queue->index()}};
    idataInfo.size = imgAsset.width * imgAsset.height * imgAsset.channels * sizeof(uint8_t);
    idataInfo.data = imgAsset.data;
    _imgdataBuffer = std::shared_ptr<RHIBuffer>(_device->createBuffer(idataInfo));

    ImageInfo imageInfo{};
    imageInfo.format = Format::RGBA8_UNORM;
    imageInfo.usage = ImageUsage::SAMPLED | ImageUsage::TRANSFER_DST;
    imageInfo.sliceCount = 1;
    imageInfo.mipCount = 1;
    imageInfo.queueAccess = {_queue->index()};
    imageInfo.extent.x = imgAsset.width;
    imageInfo.extent.y = imgAsset.height;
    imageInfo.extent.z = 1;
    imageInfo.intialLayout = ImageLayout::UNDEFINED;
    _image = std::shared_ptr<RHIImage>(_device->createImage(imageInfo));

    loader.free(std::move(imgAsset));

    ImageViewInfo imgViewInfo{};
    imgViewInfo.aspectMask = AspectMask::COLOR;
    imgViewInfo.format = imageInfo.format;
    imgViewInfo.image = _image.get();
    imgViewInfo.type = ImageViewType::IMAGE_VIEW_2D;
    imgViewInfo.range = {AspectMask::COLOR, 0, 1, 0, 1};
    _imageView = std::shared_ptr<RHIImageView>(_device->createImageView(imgViewInfo));

    SamplerInfo samplerInfo{};
    samplerInfo.minFilter = Filter::LINEAR;
    samplerInfo.magFilter = Filter::LINEAR;
    _sampler = _device->getSampler(samplerInfo);

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
    vertInfo.size = sizeof(rcubeVertices);
    _vertexBuffer = std::shared_ptr<RHIBuffer>(_device->createBuffer(vertInfo));

    BufferInfo uniformInfo{};
    uniformInfo.bufferUsage = BufferUsage::UNIFORM | BufferUsage::TRANSFER_DST;
    uniformInfo.memUsage = MemoryUsage::DEVICE_ONLY;
    uniformInfo.size = 128;
    _uniformBuffer = std::shared_ptr<RHIBuffer>(_device->createBuffer(uniformInfo));

    ShaderSourceInfo vertShaderInfo{};
    vertShaderInfo.sourcePath = "/rcubeVertStr";
    vertShaderInfo.stage.source = rcubeVertStr;
    vertShaderInfo.stage.stage = ShaderStage::VERTEX;
    _vertShader = std::shared_ptr<RHIShader>(_device->createShader(vertShaderInfo));

    ShaderSourceInfo fragShaderInfo{};
    fragShaderInfo.sourcePath = "/rcubeFragStr";
    fragShaderInfo.stage.source = rcubeFragStr;
    fragShaderInfo.stage.stage = ShaderStage::FRAGMENT;
    _fragShader = std::shared_ptr<RHIShader>(_device->createShader(fragShaderInfo));

    DescriptorSetLayoutInfo descriptorSetLayoutInfo{};
    auto& uniformBinding = descriptorSetLayoutInfo.descriptorBindings.emplace_back();
    uniformBinding.binding = 0;
    uniformBinding.count = 1;
    uniformBinding.type = DescriptorType::UNIFORM_BUFFER;
    uniformBinding.visibility = ShaderStage::VERTEX;
    auto& textureBinding = descriptorSetLayoutInfo.descriptorBindings.emplace_back();
    textureBinding.binding = 1;
    textureBinding.count = 1;
    textureBinding.type = DescriptorType::SAMPLED_IMAGE;
    textureBinding.visibility = ShaderStage::FRAGMENT;
    auto& samplerBinding = descriptorSetLayoutInfo.descriptorBindings.emplace_back();
    samplerBinding.binding = 2;
    samplerBinding.count = 1;
    samplerBinding.type = DescriptorType::SAMPLER;
    samplerBinding.visibility = ShaderStage::FRAGMENT;
    _descriptorSetLayout = std::shared_ptr<RHIDescriptorSetLayout>(_device->createDescriptorSetLayout(descriptorSetLayoutInfo));

    PipelineLayoutInfo layoutInfo{};
    layoutInfo.setLayouts.emplace_back(_descriptorSetLayout.get());
    _pipelineLayout = std::shared_ptr<RHIPipelineLayout>(_device->createPipelineLayout(layoutInfo));

    VertexLayout vertLayout{};
    VertexAttribute pos{0, 0, Format::RGB32_SFLOAT, 0};
    vertLayout.vertexAttrs.emplace_back(pos);
    VertexAttribute uv{1, 0, Format::RG32_SFLOAT, 3 * sizeof(float)};
    vertLayout.vertexAttrs.emplace_back(uv);
    VertexBufferAttribute vertBufferAttribute{0, 5 * sizeof(float), InputRate::PER_VERTEX};
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
    pipelineInfo.rasterizationInfo.cullMode = FaceMode::FRONT;
    pipelineInfo.colorBlendInfo = blendInfo;
    pipelineInfo.multisamplingInfo = msInfo;
    _pipeline = std::shared_ptr<RHIGraphicsPipeline>(_device->createGraphicsPipeline(pipelineInfo));

    DescriptorPoolInfo descPoolInfo = makeDescriptorPoolInfo({_descriptorSetLayout.get()});
    _descriptorPool = std::shared_ptr<RHIDescriptorPool>(_device->createDescriptorPool(descPoolInfo));

    DescriptorSetInfo descInfo{};
    descInfo.layout = _descriptorSetLayout.get();
    auto& descriptorBinding = descInfo.bindingInfos.emplace_back();
    auto& bufferBinding = descriptorBinding.bufferBindings.emplace_back();
    bufferBinding.arrayElement = 0;
    bufferBinding.binding = 0;
    bufferBinding.type = DescriptorType::UNIFORM_BUFFER;
    bufferBinding.buffers = {{0, 128, _uniformBuffer.get()}};
    auto& imgBingding = descriptorBinding.imageBindings.emplace_back();
    imgBingding.arrayElement = 0;
    imgBingding.binding = 1;
    imgBingding.type = DescriptorType::SAMPLED_IMAGE;
    imgBingding.imageViews = {{ImageLayout::SHADER_READ_ONLY_OPTIMAL, _imageView.get()}};
    auto& smpBinding = descriptorBinding.samplerBindings.emplace_back();
    smpBinding.arrayElement = 0;
    smpBinding.binding = 2;
    smpBinding.samplers = {{_sampler}};

    _descriptorSet = std::shared_ptr<RHIDescriptorSet>(_descriptorPool->makeDescriptorSet(descInfo));
}

void RotatingCube::show() {
    _swaphchain->aquire();
    auto* scImageView = _swaphchain->swapchainImageView();

    if (_commandBuffers.find(scImageView) == _commandBuffers.end()) {
        _commandBuffers[scImageView] = std::shared_ptr<RHICommandBuffer>(_commandPool->makeCommandBuffer({}));
    }
    auto commandbuffer = _commandBuffers[scImageView];

    auto width = scImageView->viewInfo().image->info().extent.x;
    auto height = scImageView->viewInfo().image->info().extent.y;

    commandbuffer->reset();

    commandbuffer->begin({});

    if (_firstFrame) {
        std::shared_ptr<RHIBlitEncoder> blitEncoder(commandbuffer->makeBlitEncoder());
        blitEncoder->updateBuffer(_vertexBuffer.get(), 0, rcubeVertices, sizeof(rcubeVertices));

        _udata[0] = glm::rotate(glm::mat4(1.0f), glm::radians(0.0f), glm::vec3(1.0, 1.0, 1.0));
        _udata[1] = glm::perspectiveFovLH_ZO(glm::pi<float>() / 3.f, (float)width, (float)height, 0.1f, 100.0f);

        blitEncoder->updateBuffer(_uniformBuffer.get(), 0, &_udata[0], 128);

        BufferBarrierInfo bufferBarrierInfo{};
        bufferBarrierInfo.buffer = _vertexBuffer.get();
        bufferBarrierInfo.srcStage = PipelineStage::TRANSFER;
        bufferBarrierInfo.dstStage = PipelineStage::VERTEX_INPUT;
        bufferBarrierInfo.srcQueueIndex = _queue->index();
        bufferBarrierInfo.dstQueueIndex = _queue->index();
        bufferBarrierInfo.size = _vertexBuffer->info().size;
        commandbuffer->appendBufferBarrier(bufferBarrierInfo);

        bufferBarrierInfo.buffer = _uniformBuffer.get();
        bufferBarrierInfo.size = _uniformBuffer->info().size;
        bufferBarrierInfo.dstStage = PipelineStage::VERTEX_SHADER;
        commandbuffer->appendBufferBarrier(bufferBarrierInfo);

        BufferImageCopyRegion region{};
        region.bufferSize = _uniformBuffer->info().size;
        region.bufferImageHeight = 0;
        region.bufferRowLength = 0;
        region.imageExtent = _image->info().extent;

        ImageBarrierInfo imgBarrierInfo{};
        imgBarrierInfo.image = _image.get();
        imgBarrierInfo.dstStage = PipelineStage::TRANSFER;
        imgBarrierInfo.srcAccessFlag = AccessFlags::NONE;
        imgBarrierInfo.dstAccessFlag = AccessFlags::TRANSFER_WRITE;
        imgBarrierInfo.oldLayout = ImageLayout::UNDEFINED;
        imgBarrierInfo.newLayout = ImageLayout::TRANSFER_DST_OPTIMAL;
        imgBarrierInfo.srcQueueIndex = _queue->index();
        imgBarrierInfo.dstQueueIndex = _queue->index();
        imgBarrierInfo.range = {AspectMask::COLOR, 0, 1, 0, 1};
        commandbuffer->appendImageBarrier(imgBarrierInfo);
        commandbuffer->applyBarrier(DependencyFlags::BY_REGION);

        blitEncoder->copyBufferToImage(_imgdataBuffer.get(), _image.get(), ImageLayout::TRANSFER_DST_OPTIMAL, &region, 1);

        imgBarrierInfo.image = _image.get();
        imgBarrierInfo.srcStage = PipelineStage::TRANSFER;
        imgBarrierInfo.dstStage = PipelineStage::FRAGMENT_SHADER;
        imgBarrierInfo.srcAccessFlag = AccessFlags::TRANSFER_WRITE;
        imgBarrierInfo.dstAccessFlag = AccessFlags::SHADER_READ;
        imgBarrierInfo.oldLayout = ImageLayout::TRANSFER_DST_OPTIMAL;
        imgBarrierInfo.newLayout = ImageLayout::SHADER_READ_ONLY_OPTIMAL;
        imgBarrierInfo.srcQueueIndex = _queue->index();
        imgBarrierInfo.dstQueueIndex = _queue->index();
        imgBarrierInfo.range = {AspectMask::COLOR, 0, 1, 0, 1};
        commandbuffer->appendImageBarrier(imgBarrierInfo);

        _firstFrame = false;
    } else {
        BufferBarrierInfo bufferBarrierInfo{};
        bufferBarrierInfo.buffer = _uniformBuffer.get();
        bufferBarrierInfo.srcStage = PipelineStage::VERTEX_SHADER;
        bufferBarrierInfo.dstStage = PipelineStage::TRANSFER;
        bufferBarrierInfo.offset = 0;
        bufferBarrierInfo.size = 16;
        commandbuffer->appendBufferBarrier(bufferBarrierInfo);
        commandbuffer->applyBarrier(DependencyFlags::BY_REGION); 

        std::shared_ptr<RHIBlitEncoder> blitEncoder(commandbuffer->makeBlitEncoder());
        _udata[0] = glm::rotate(_udata[0], 0.0002f, glm::vec3(1.0, 1.0, 1.0));
        auto modelMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, 2.0));
        modelMat = modelMat * _udata[0];
        blitEncoder->updateBuffer(_uniformBuffer.get(), 0, &modelMat[0], 64);

        bufferBarrierInfo.srcStage = PipelineStage::TRANSFER;
        bufferBarrierInfo.dstStage = PipelineStage::VERTEX_SHADER;
        commandbuffer->appendBufferBarrier(bufferBarrierInfo);
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
    ClearColor clearColor{0.5, 0.5, 0.5, 1.0};
    beginInfo.clearColors = &clearColor;

    encoder->beginRenderPass(beginInfo);
    Viewport vp{0, 0, frameBuffer->info().width, frameBuffer->info().height, 0.0, 1.0};
    encoder->setViewport(vp);
    encoder->setScissor({0, 0, frameBuffer->info().width, frameBuffer->info().height});
    encoder->bindPipeline(_pipeline.get());
    encoder->bindDescriptorSet(_descriptorSet.get(), 0, nullptr, 0);
    encoder->bindVertexBuffer(_vertexBuffer.get(), 0);
    encoder->draw(36, 1, 0, 0);
    encoder->endRenderPass();

    commandbuffer->commit(_queue);

    _queue->submit();
    _swaphchain->present();
}

} // namespace raum::sample