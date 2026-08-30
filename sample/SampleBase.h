#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "RHIDefine.h"
#include "core/math.h"

namespace raum::framework {
class Director;
}

namespace raum::graph {
class Pipeline;
class ResourceGraph;
class SceneGraph;
} // namespace raum::graph

namespace raum::sample {

using LoadingProgressCallback = std::function<void(float, std::string_view)>;

struct CameraControlState {
    Vec3f position{0.0f};
    float yawDegrees{0.0f};
    float pitchDegrees{0.0f};
    float verticalFovDegrees{60.0f};
};

class SampleBase {
public:
    SampleBase(framework::Director* director, std::string name);
    virtual ~SampleBase() = default;

    SampleBase(const SampleBase&) = delete;
    SampleBase& operator=(const SampleBase&) = delete;

    void load(const LoadingProgressCallback& progress = {});
    void initialize();
    void activate();
    void render();
    void deactivate();

    bool initialized() const { return _initialized; }
    bool active() const { return _active; }
    const std::string& name() const { return _name; }

    virtual std::optional<CameraControlState> cameraControlState() const { return std::nullopt; }
    virtual void applyCameraControlState(const CameraControlState&) {}

protected:
    virtual void onLoad(const LoadingProgressCallback&) {}
    virtual void onInitialize() = 0;
    virtual void onRender() = 0;
    virtual void onActivated() {}
    virtual void onDeactivated() {}

    framework::Director& director() const;
    graph::Pipeline& pipeline() const;
    graph::ResourceGraph& resourceGraph() const;
    graph::SceneGraph& sceneGraph() const;
    rhi::DevicePtr device() const { return _device; }
    rhi::SwapchainPtr swapchain() const { return _swapchain; }

    uint32_t viewportWidth() const;
    uint32_t viewportHeight() const;

    // Resource names are scoped per sample so lazily initialized samples do
    // not accidentally reuse incompatible images or buffers.
    const std::string& resource(std::string_view localName) const;
    void ensureSwapchain(std::string_view localName);
    void ensureBuffer(std::string_view localName, uint32_t size, rhi::BufferUsage usage);
    void ensureImage(
        std::string_view localName,
        rhi::ImageUsage usage,
        uint32_t width,
        uint32_t height,
        rhi::Format format);
    void ensureViewportImage(std::string_view localName, rhi::ImageUsage usage, rhi::Format format);
    void ensureSampler(std::string_view localName, const rhi::SamplerInfo& info);
    void resizeViewportResources();

    void loadScene(
        const std::filesystem::path& filePath,
        std::string_view localName = "scene",
        const LoadingProgressCallback& progress = {});

    // Tracked nodes are automatically disabled/enabled when switching samples.
    void trackSceneNode(std::string_view nodeName);

private:
    void setTrackedSceneNodesEnabled(bool enabled);

    framework::Director* _director{nullptr};
    graph::Pipeline* _pipeline{nullptr};
    rhi::DevicePtr _device;
    rhi::SwapchainPtr _swapchain;
    std::string _name;
    mutable std::unordered_map<std::string, std::string> _resourceNames;
    std::vector<std::string> _viewportImageNames;
    std::vector<std::string> _sceneNodeNames;
    bool _loaded{false};
    bool _initialized{false};
    bool _active{false};
};

} // namespace raum::sample
