#pragma once
#include <cstdlib>
#include <iostream>
#include "GraphSample.h"
#include "VirtualTexture.h"
#include "WindowEvent.h"
#include "World.h"
#include "common.h"

namespace raum {
using platform::Window;

constexpr uint32_t s_width = 1080u;
constexpr uint32_t s_height = 720u;

class Sample {
public:
    Sample(int argc, char** argv) {
        _world = new framework::World();
        auto& director = _world->director();
        auto device = director.device();

        _window = std::make_shared<platform::Window>(argc, argv, s_width, s_height, device->instance());
        _tick = platform::TickFunction{[&](std::chrono::milliseconds miliSec) {
            this->show();
        }};
        _window->registerPollEvents(&_tick);

        _world->attachWindow(_window);

        _world->run();

        auto swapchain = director.swapchain();

        auto resizeHandler = [&, swapchain](uint32_t w, uint32_t h) {
            swapchain->resize(w, h, _window->surface());
        };
        _resizeListener.add(resizeHandler);

        auto closeHandler = [&]() {
            _resizeListener.remove();
            _closeListener.remove();
        };
        _closeListener.add(closeHandler);

        _samples = {
            std::make_shared<sample::GraphSample>(&_world->director()),
//            std::make_shared<sample::VirtualTextureSample>(&_world->director()),
        };
        _inited.resize(_samples.size(), 0);

        _samples[_currIndex]->init();
        _inited[_currIndex] = 1;
    }

    ~Sample() {
    }

    void show() {
        if (_currIndex < _samples.size()) {
            _samples[_currIndex]->show();
        }
    }

    platform::WindowPtr window() {
        return _window;
    }

    void changeSample(uint32_t index) {
        if (index == _currIndex) return;
        _samples[_currIndex]->hide();
        if (!_inited[index]) {
            _samples[index]->init();
            _inited[index] = 1;
        }
        _currIndex = index;
        auto ppl = _world->director().pipeline();
        ppl->graphScheduler().needWarmUp();
    }

    const std::vector<std::shared_ptr<sample::SampleBase>>& samples() const {
        return _samples;
    }

private:
    uint32_t _currIndex{0};
    std::vector<std::shared_ptr<sample::SampleBase>> _samples;
    std::vector<uint32_t> _inited;
    platform::WindowPtr _window;
    framework::World* _world{nullptr};
    framework::EventListener<framework::ResizeEventTag> _resizeListener;
    framework::EventListener<framework::CloseEventTag> _closeListener;
    platform::TickFunction _tick;
};

} // namespace raum
