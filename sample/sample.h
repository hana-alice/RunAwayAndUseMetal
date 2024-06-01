#include <cstdlib>
#include <iostream>
#include "GraphSample.h"
#include "RHIManager.h"
#include "common.h"
#include "window.h"

namespace raum {
using platform::Window;

constexpr uint32_t width = 1080u;
constexpr uint32_t height = 720u;

class Sample {
public:
    Sample(int argc, char** argv) {
        _window = std::make_shared<Window>(argc, argv, width, height);
        _device = std::shared_ptr<rhi::RHIDevice>(loadRHI(rhi::API::VULKAN), rhi::unloadRHI);

        auto pxSize = _window->pixelSize();
        rhi::SwapchainInfo scInfo{pxSize.width, pxSize.height, rhi::SyncType::IMMEDIATE, _window->handle()};
        _swapchain = std::shared_ptr<rhi::RHISwapchain>(_device->createSwapchain(scInfo));

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
        _window->show();
        _window->mainLoop();
    }

private:
    platform::WindowPtr _window;
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;
    std::vector<std::shared_ptr<sample::SampleBase>> _samples;
};

} // namespace raum
