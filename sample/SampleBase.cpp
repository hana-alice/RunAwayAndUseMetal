#include "SampleBase.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "Director.h"
#include "Pipeline.h"
#include "ResourceGraph.h"
#include "SceneGraph.h"
#include "SceneSerializer.h"

namespace raum::sample {

SampleBase::SampleBase(framework::Director* director, std::string name)
: _director(director), _name(std::move(name)) {
    if (!_director) {
        throw std::invalid_argument("A sample requires a Director");
    }
    const auto pipeline = _director->pipeline();
    if (!pipeline) {
        throw std::logic_error("The Director must be attached to a window before samples are created");
    }
    _pipeline = pipeline.get();
    _device = _director->device();
    _swapchain = _director->swapchain();
}

void SampleBase::initialize() {
    if (_initialized) {
        return;
    }
    onInitialize();
    _initialized = true;
    if (!_active) {
        setTrackedSceneNodesEnabled(false);
    }
}

void SampleBase::activate() {
    initialize();
    if (_active) {
        return;
    }
    setTrackedSceneNodesEnabled(true);
    _active = true;
    try {
        onActivated();
    } catch (...) {
        _active = false;
        setTrackedSceneNodesEnabled(false);
        throw;
    }
}

void SampleBase::render() {
    if (_active) {
        onRender();
    }
}

void SampleBase::deactivate() {
    if (!_active) {
        return;
    }
    _active = false;
    try {
        onDeactivated();
    } catch (...) {
        setTrackedSceneNodesEnabled(false);
        throw;
    }
    setTrackedSceneNodesEnabled(false);
}

framework::Director& SampleBase::director() const {
    return *_director;
}

graph::Pipeline& SampleBase::pipeline() const {
    return *_pipeline;
}

graph::ResourceGraph& SampleBase::resourceGraph() const {
    return _pipeline->resourceGraph();
}

graph::SceneGraph& SampleBase::sceneGraph() const {
    return _director->sceneGraph();
}

uint32_t SampleBase::viewportWidth() const {
    return _swapchain->width();
}

uint32_t SampleBase::viewportHeight() const {
    return _swapchain->height();
}

const std::string& SampleBase::resource(std::string_view localName) const {
    auto [iter, inserted] = _resourceNames.try_emplace(std::string{localName});
    if (inserted) {
        iter->second.reserve(_name.size() + localName.size() + 1);
        iter->second.append(_name);
        iter->second.push_back('/');
        iter->second.append(localName);
    }
    return iter->second;
}

void SampleBase::ensureSwapchain(std::string_view localName) {
    const auto& name = resource(localName);
    if (!resourceGraph().contains(name)) {
        resourceGraph().import(name, _swapchain);
    }
}

void SampleBase::ensureBuffer(std::string_view localName, uint32_t size, rhi::BufferUsage usage) {
    const auto& name = resource(localName);
    if (!resourceGraph().contains(name)) {
        resourceGraph().addBuffer(name, size, usage);
    }
}

void SampleBase::ensureImage(
    std::string_view localName,
    rhi::ImageUsage usage,
    uint32_t width,
    uint32_t height,
    rhi::Format format) {
    const auto& name = resource(localName);
    if (!resourceGraph().contains(name)) {
        resourceGraph().addImage(name, usage, width, height, format);
    }
}

void SampleBase::ensureViewportImage(std::string_view localName, rhi::ImageUsage usage, rhi::Format format) {
    ensureImage(localName, usage, viewportWidth(), viewportHeight(), format);
    const std::string key{localName};
    if (std::ranges::find(_viewportImageNames, key) == _viewportImageNames.end()) {
        _viewportImageNames.emplace_back(key);
    }
}

void SampleBase::ensureSampler(std::string_view localName, const rhi::SamplerInfo& info) {
    const auto& name = resource(localName);
    if (!resourceGraph().contains(name)) {
        resourceGraph().addSampler(name, info);
    }
}

void SampleBase::resizeViewportResources() {
    for (const auto& localName : _viewportImageNames) {
        resourceGraph().updateImage(resource(localName), viewportWidth(), viewportHeight());
    }
}

void SampleBase::loadScene(const std::filesystem::path& filePath, std::string_view localName) {
    const auto loadedNodes = asset::serialize::loadScoped(
        sceneGraph(),
        filePath,
        resource(localName),
        device());
    for (const auto& nodeName : loadedNodes) {
        trackSceneNode(nodeName);
    }
}

void SampleBase::trackSceneNode(std::string_view nodeName) {
    const std::string name{nodeName};
    if (std::ranges::find(_sceneNodeNames, name) == _sceneNodeNames.end()) {
        _sceneNodeNames.emplace_back(name);
    }
}

void SampleBase::setTrackedSceneNodesEnabled(bool enabled) {
    for (const auto& name : _sceneNodeNames) {
        if (enabled) {
            sceneGraph().enable(name);
        } else {
            sceneGraph().disable(name);
        }
    }
}

} // namespace raum::sample
