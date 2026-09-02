#pragma once
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include "SceneGraph.h"

namespace raum::asset::serialize {

using ProgressCallback = std::function<void(float, std::string_view)>;

void deserialize(graph::SceneGraph& sg, const std::filesystem::path& filePath);

void load(graph::SceneGraph& sg,
          const std::filesystem::path& filePath,
          std::string_view sceneName,
          rhi::DevicePtr device);

void load(graph::SceneGraph& sg,
          const std::filesystem::path& filePath,
          rhi::DevicePtr device);

// Loads a scene into a caller-owned node namespace and returns every node
// created in that namespace. This allows independent scene instances to
// coexist in one SceneGraph.
std::vector<std::string> loadScoped(graph::SceneGraph& sg,
                                    const std::filesystem::path& filePath,
                                    std::string_view scope,
                                    rhi::DevicePtr device,
                                    const ProgressCallback& progress = {});

void loadSkybox(const std::filesystem::path& filePath,
                rhi::DevicePtr device);
} // namespace raum::asset::serialize
