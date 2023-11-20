#pragma once

#include <cstdlib>
#include <iostream>
#include "Triangle.hpp"
#include "VKDevice.h"
namespace raum {
using platform::NativeWindow;
using rhi::VKDevice;
class Sample {
public:
    Sample() {
        _window = std::make_shared<NativeWindow>(800, 600);
        _device = VKDevice::getInstance();
    }

    ~Sample() {}

    void show() {
        _window->mainLoop();
    }

private:
    std::shared_ptr<NativeWindow> _window;
    VKDevice* _device;
};

} // namespace raum

int main() {
    raum::Sample sample{};
    sample.show();
    return 0;
}