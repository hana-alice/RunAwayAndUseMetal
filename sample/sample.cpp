#pragma once

#include <cstdlib>
#include <iostream>
#include "Model.hpp"
#include "Triangle.hpp"
#include "RHIManager.h"
#include "RHICommandBuffer.h"
#include "RHIImage.h"
#include "RHIImageView.h"
namespace raum {
using platform::NativeWindow;
using namespace rhi;

constexpr uint32_t width = 1080u;
constexpr uint32_t height = 720u;

class Sample {
public:
    Sample() {
        _window = std::make_shared<NativeWindow>(width, height);
        _device = loadRHI(API::VULKAN);
        _swaphchain = _device->createSwapchain(SwapchainInfo{width, height, SyncType::RELAX, _window->handle()});
        _window->registerPollEvents(std::bind(&Sample::render, this));
        _queue = _device->getQueue(QueueInfo{QueueType::GRAPHICS});
        _commandBuffer = _queue->makeCommandBuffer({});
    }

    ~Sample() {}

    void show() {
        _window->mainLoop();
    }

private:
    void render() {
        _swaphchain->aquire();
        _commandBuffer->reset();

        _commandBuffer->begin({});
        ImageBarrierInfo imageBarrierInfo{};
        imageBarrierInfo.image = _swaphchain->swapchainImageView()->image();
        imageBarrierInfo.oldLayout = ImageLayout::UNDEFINED;
        imageBarrierInfo.newLayout = ImageLayout::PRESENT;
        imageBarrierInfo.srcAccessFlag = AccessFlags::NONE;
        imageBarrierInfo.dstAccessFlag = AccessFlags::NONE;
        imageBarrierInfo.srcQueueIndex = _queue->index();
        imageBarrierInfo.dstQueueIndex = _queue->index();
        imageBarrierInfo.srcStage = PipelineStage::TOP_OF_PIPE;
        imageBarrierInfo.dstStage = PipelineStage::BOTTOM_OF_PIPE;
        imageBarrierInfo.range = _swaphchain->swapchainImageView()->viewInfo().range;

        _commandBuffer->appendImageBarrier(imageBarrierInfo);
        _commandBuffer->applyBarrier(DependencyFlags::BY_REGION);
        _commandBuffer->commit();

        _queue->submit();
        _swaphchain->present();
    }
    std::shared_ptr<NativeWindow> _window;
    RHIDevice* _device;
    RHISwapchain* _swaphchain;
    RHIQueue* _queue;
    RHICommandBuffer* _commandBuffer;
};

} // namespace raum

int main() {
    raum::Sample sample{};
    sample.show();
    return 0;
}