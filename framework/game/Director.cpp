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
    const auto& pxSize = window->size();
    rhi::SwapchainSurfaceInfo scInfo{pxSize.width, pxSize.height, rhi::SyncType::VSYNC, window->handle()};
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

void Director::addPreRenderTask(RenderTask* tick) {
    _preRenderTasks.emplace_back(tick);
}

void Director::addPostRenderTask(RenderTask* tick) {
    _postRenderTasks.emplace_back(tick);
}

void Director::removePreRenderTask(RenderTask* tick) {
    for(auto it=  _preRenderTasks.begin(); it != _preRenderTasks.end();) {
        if(*it == tick) {
            it = _preRenderTasks.erase(it);
            break;
        } else {
            ++it;
        }
    }
}

void Director::removePostRenderTask(RenderTask* tick) {
    for(auto it=  _postRenderTasks.begin(); it != _postRenderTasks.end();) {
        if(*it == tick) {
            it = _postRenderTasks.erase(it);
            break;
        } else {
            ++it;
        }
    }
}

void Director::preRender(std::chrono::milliseconds miliSec, rhi::CommandBufferPtr cmd) {
    for(auto* task : _preRenderTasks) {
        (*task)(miliSec, cmd, _device);
    }
}

void Director::postRender(std::chrono::milliseconds miliSec, rhi::CommandBufferPtr cmd) {
    for(auto* task : _postRenderTasks) {
        (*task)(miliSec, cmd, _device);
    }
}

void Director::update(std::chrono::milliseconds milisec) {
    auto* queue = _device->getQueue({rhi::QueueType::GRAPHICS});

    auto res = _swapchain->acquire();
    raum_check(res, "");
    auto* acquireSem = _swapchain->getAvailableSemaphore();

    auto cmd = _cmds[_swapchain->imageIndex()];

    cmd->reset();
    cmd->enqueue(queue);
    cmd->begin({});

    preRender(milisec, cmd);
    _pipeline->run(cmd);
    postRender(milisec, cmd);

    cmd->commit();
    queue->addWait(acquireSem);

    auto* presentSignalSem = _swapchain->getSignalPresentSemaphore();
    queue->addSignal(presentSignalSem);
    queue->submit(true);

    _swapchain->present();
}

void Director::run() {
    _tickID = _window->addTick([&](const std::chrono::milliseconds& miliSec){
        this->update(miliSec);
    });
}

Director::~Director() {
    _window->removeTick(_tickID);
}

} // namespace raum::framework