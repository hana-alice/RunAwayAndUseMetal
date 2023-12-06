#pragma once

#include <cstdlib>
#include <iostream>
#include "Triangle.hpp"
#include "VKDevice.h"
#include "VKSwapchain.h"
namespace raum {
using platform::NativeWindow;
using namespace rhi;

constexpr uint32_t width = 1080u;
constexpr uint32_t height = 720u;

class Sample {
public:
    Sample() {
        _window = std::make_shared<NativeWindow>(width, height);
        _device = Device::getInstance();
        _swaphchain = _device->createSwapchain(SwapchainInfo{width, height, SyncType::RELAX, _window->handle()});
        
    }

    ~Sample() {}

    void show() {
        _window->mainLoop();
    }

private:
    std::shared_ptr<NativeWindow> _window;
    Device* _device;
    Swapchain* _swaphchain;
};

} // namespace raum

int main() {
    raum::Sample sample{};
    sample.show();
    return 0;
}