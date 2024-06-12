#pragma  once
#include <cstdlib>
#include <iostream>
#include "GraphSample.h"
#include "RHIManager.h"
#include "common.h"
#include "window.h"
#include "WindowEvent.h"
//#include "AnimationModel.h"

namespace raum {
using platform::Window;

constexpr uint32_t s_width = 1080u;
constexpr uint32_t s_height = 720u;

class Sample {
public:
    Sample(int argc, char** argv) {
        _device = std::shared_ptr<rhi::RHIDevice>(loadRHI(rhi::API::VULKAN), rhi::unloadRHI);
        _window = std::make_shared<platform::Window>(argc, argv, s_width, s_height, _device->instance());

        auto pxSize = _window->pixelSize();
        rhi::SwapchainSurfaceInfo scInfo{pxSize.width, pxSize.height, rhi::SyncType::IMMEDIATE, _window->surface()};
        _swapchain = std::shared_ptr<rhi::RHISwapchain>(_device->createSwapchain(scInfo));

        _graphScheduler = std::make_shared<graph::GraphScheduler>(_device, _swapchain);

        auto resizeHandler = [&](uint32_t w, uint32_t h){
            _swapchain->resize(w, h, _window->surface());
        };
        _resizeListener.add(resizeHandler);

        auto closeHandler = [&]() {
            _resizeListener.remove();
            _closeListener.remove();
        };
        _closeListener.add(closeHandler);

        _samples = {
            std::make_shared<sample::GraphSample>(_device, _swapchain, _graphScheduler),
//            std::make_shared<sample::AnimationModel>(_device, _swapchain, _graphScheduler),
        };
        _inited.resize(_samples.size(), 0);

        _samples[_currIndex]->init();
        _inited[_currIndex] = 1;
    }

    ~Sample() {
    }

    void show() {
        if(_currIndex < _samples.size()) {
            _samples[_currIndex]->show();
        }
    }

    platform::WindowPtr window() {
        return _window;
    }

    void changeSample(uint32_t index) {
        if(index == _currIndex) return;
        _samples[_currIndex]->hide();
        if(!_inited[index]) {
            _samples[index]->init();
            _inited[index] = 1;
        }
        _currIndex = index;
        _graphScheduler->needWarmUp();
    }

    const std::vector<std::shared_ptr<sample::SampleBase>>& samples() const {
        return _samples;
    }

private:
    uint32_t _currIndex{0};
    std::vector<std::shared_ptr<sample::SampleBase>> _samples;
    std::vector<uint32_t> _inited;
    platform::WindowPtr _window;
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;
    graph::GraphSchedulerPtr  _graphScheduler;
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
    framework::EventListener<framework::CloseEventTag> _closeListener;
};

} // namespace raum
