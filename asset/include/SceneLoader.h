#pragma once
#include <filesystem>
#include "Scene.h"
#include "SceneGraph.h"
#include "RHIDevice.h"

namespace raum::asset {


class SceneLoader {
public:
    SceneLoader() = delete;
    SceneLoader(rhi::DevicePtr device);

    ~SceneLoader() = default;

    void loadFlat(const std::filesystem::path& filePath);

    const scene::Model& modelData() { return _data; }

private:
    scene::Model _data;
    rhi::DevicePtr  _device;
};

} // namespace raum::framework::asset