//
// Created by zeqia on 2024/7/11.
//

#pragma once
#include "SceneGraph.h"
#include "RenderGraph.h"
#include "ResourceGraph.h"
#include <filesystem>
#include "Pipeline.h"
#include "window.h"
namespace raum::framework {

class Director {
public:
    explicit Director();
    ~Director();

    void attachWindow(platform::WindowPtr window);

    void loadScene(std::filesystem::path p, std::string_view name);
    void unloadScene(std::string_view name);

    void enableScene(std::string_view name);
    void disableScene(std::string_view name);

    graph::SceneGraph& sceneGraph() { return *_sceneGraph; }

    void run();

    graph::PipelinePtr pipeline() { return _pipeline; }
    rhi::DevicePtr device() { return _device; }
    rhi::SwapchainPtr swapchain() { return _swapchain; }
private:
    void update(std::chrono::milliseconds miliSec);
    graph::SceneGraphPtr _sceneGraph;
    graph::ShaderGraphPtr _shaderGraph;
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;

    rhi::CommandPoolPtr _cmdPool;
    std::array<rhi::CommandBufferPtr, rhi::FRAMES_IN_FLIGHT> _cmds;

    graph::PipelinePtr _pipeline;
    platform::WindowPtr _window;

    platform::TickFunction _tick;
};

} // namespace raum::framework
