#include <cstdlib>
#include <iostream>
#include "RotatingCube.hpp"
#include "Triangle.hpp"
#include "common.h"
#include "window.h"
#include "GraphSample.hpp"

namespace raum {
using platform::NativeWindow;
using namespace rhi;

constexpr uint32_t width = 1080u;
constexpr uint32_t height = 720u;

class Sample {
public:
    Sample() {
        _window = std::make_shared<NativeWindow>(width, height);
        _device = std::shared_ptr<RHIDevice>(loadRHI(API::VULKAN), unloadRHI);

        SwapchainInfo scInfo{width, height, SyncType::IMMEDIATE, _window->handle()};
        _swapchain = std::shared_ptr<RHISwapchain>(_device->createSwapchain(scInfo));

        _samples = {
//            std::make_shared<sample::RotatingCube>(_device, _swapchain),
            std::make_shared<sample::GraphSample>(_device, _swapchain),
        };

        for (const auto& s : _samples) {
            _window->registerPollEvents(std::bind(&sample::SampleBase::show, s));
        }
    }

    ~Sample() {
    }

    void show() {
        _window->mainLoop();
    }

private:
    std::shared_ptr<NativeWindow> _window;
    std::shared_ptr<RHIDevice> _device;
    std::shared_ptr<RHISwapchain> _swapchain;
    std::vector<std::shared_ptr<sample::SampleBase>> _samples;
};

} // namespace raum

int main() {
    raum::Sample sample{};
    sample.show();
    return 0;
}