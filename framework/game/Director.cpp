//
// Created by zeqia on 2024/7/11.
//

#include "Director.h"
#include "SceneSerializer.h"
#include "RHICommandBuffer.h"
#include "RHIManager.h"
#include "BuiltinRes.h"

namespace raum::framework {

Director::Director() {
    _device = std::shared_ptr<rhi::RHIDevice>(rhi::loadRHI(rhi::API::VULKAN), rhi::unloadRHI);

    _sceneGraph = std::make_shared<graph::SceneGraph>();
    _shaderGraph = std::make_shared<graph::ShaderGraph>(_device);
    _cmdPool = rhi::CommandPoolPtr(_device->createCoomandPool({_device->getQueue({rhi::QueueType::GRAPHICS})->index()}));
    for(auto& cmd : _cmds) {
        cmd = rhi::CommandBufferPtr(_cmdPool->makeCommandBuffer({}));
    }
    asset::BuiltinRes::initialize(*_shaderGraph, _device);
}

void Director::attachWindow(platform::WindowPtr window) {
    _window = window;
    auto pxSize = window->pixelSize();
    rhi::SwapchainSurfaceInfo scInfo{pxSize.width, pxSize.height, rhi::SyncType::IMMEDIATE, window->surface()};
    _swapchain = std::shared_ptr<rhi::RHISwapchain>(_device->createSwapchain(scInfo));
    _pipeline = std::make_shared<graph::Pipeline>(_device, _swapchain, _sceneGraph, _shaderGraph);
}

void Director::loadScene(std::filesystem::path p, std::string_view name) {
    asset::serialize::load(*_sceneGraph, p, name, _device);
}

void Director::unloadScene(std::string_view name) {

}

void Director::enableScene(std::string_view name) {
    _sceneGraph->enable(name);
}

void Director::disableScene(std::string_view name) {
    _sceneGraph->disable(name);
}

void Director::update(std::chrono::milliseconds milisec) {
    auto* queue = _device->getQueue({rhi::QueueType::GRAPHICS});

    auto* acquireSem = _swapchain->getAvailableByAcquire();
    _swapchain->acquire();

    auto* renderSem = queue->getSignal();
    _swapchain->addWaitBeforePresent(renderSem);

    auto cmd = _cmds[_swapchain->imageIndex()];

    cmd->reset();
    cmd->enqueue(queue);
    cmd->begin({});

    _pipeline->run(cmd);

    cmd->commit();
    queue->addWait(acquireSem);
    queue->submit(true);

    _swapchain->present();
}

void Director::run() {
    _tick = platform::TickFunction{[&](std::chrono::milliseconds miliSec){
        this->update(miliSec);
    }};
    _window->registerPollEvents(&_tick);
}

Director::~Director() {
    _window->removePollEvent(&_tick);
}

} // namespace raum::framework