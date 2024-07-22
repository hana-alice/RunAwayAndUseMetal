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
using TickFunction = utils::TickFunction<std::chrono::milliseconds>;
using RenderTask = utils::TickFunction<std::chrono::milliseconds,
                                       rhi::CommandBufferPtr,
                                       rhi::DevicePtr>;

class Director {
public:
    explicit Director();
    ~Director();

    void attachWindow(platform::WindowPtr window);

    void loadScene(std::filesystem::path p, std::string_view name);
    void unloadScene(std::string_view name);

    void enableScene(std::string_view name);
    void disableScene(std::string_view name);

    void addPreRenderTask(RenderTask* tick);
    void addPostRenderTask(RenderTask* tick);
    void removePreRenderTask(RenderTask* tick);
    void removePostRenderTask(RenderTask* tick);

    graph::SceneGraph& sceneGraph() { return *_sceneGraph; }

    void run();

    graph::PipelinePtr pipeline() { return _pipeline; }
    rhi::DevicePtr device() { return _device; }
    rhi::SwapchainPtr swapchain() { return _swapchain; }
private:
    void preRender(std::chrono::milliseconds miliSec, rhi::CommandBufferPtr cmd);
    void postRender(std::chrono::milliseconds miliSec, rhi::CommandBufferPtr cmd);
    void update(std::chrono::milliseconds miliSec);
    graph::SceneGraphPtr _sceneGraph;
    graph::ShaderGraphPtr _shaderGraph;
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;

    rhi::CommandPoolPtr _cmdPool;
    std::array<rhi::CommandBufferPtr, rhi::FRAMES_IN_FLIGHT> _cmds;

    graph::PipelinePtr _pipeline;
    platform::WindowPtr _window;

    std::vector<RenderTask*> _preRenderTasks;
    std::vector<RenderTask*> _postRenderTasks;

    TickFunction _tick;
};

} // namespace raum::framework
