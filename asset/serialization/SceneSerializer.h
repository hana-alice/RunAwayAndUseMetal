#pragma once
#include <filesystem>
#include "SceneGraph.h"

namespace raum::asset::serialize {

void deserialize(graph::SceneGraph& sg, const std::filesystem::path& filePath);

void load(graph::SceneGraph& sg,
          const std::filesystem::path& filePath,
          std::string_view sceneName,
          rhi::DevicePtr device);

void load(graph::SceneGraph& sg,
          const std::filesystem::path& filePath,
          rhi::DevicePtr device);

void loadSkybox(const std::filesystem::path& filePath,
                rhi::DevicePtr device);
}