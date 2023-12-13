#pragma once

#include <cstdlib>
#include <iostream>
#include "Model.hpp"
#include "Triangle.hpp"
#include "RHIManager.h"
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
    }

    ~Sample() {}

    void show() {
        _window->mainLoop();
    }

private:
    std::shared_ptr<NativeWindow> _window;
    RHIDevice* _device;
    RHISwapchain* _swaphchain;
};

} // namespace raum

int main() {
    raum::Sample sample{};
    sample.show();
    return 0;
}