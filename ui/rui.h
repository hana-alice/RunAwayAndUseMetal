#pragma once
#include "RHIDefine.h"
#include "window.h"

namespace raum::ui {

class RUI {
public:
    ~RUI() = default;
    RUI(const RUI&) = delete;
    RUI& operator=(const RUI&) = delete;
    RUI(RUI&&) = delete;
    RUI& operator=(RUI&&) = delete;

    explicit RUI(rhi::DevicePtr device,
    platform::WindowPtr window);

private:
    rhi::DevicePtr _device;
    platform::WindowPtr _window;
};

}