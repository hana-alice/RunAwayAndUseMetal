#pragma once
#include <filesystem>
#include "Scene.h"
#include "SceneGraph.h"

namespace raum::framework::asset {

struct MeshData {
    rhi::IndexType indexType{rhi::IndexType::HALF};
    uint32_t materialIndex{0};
    std::vector<uint8_t> indices;
    std::vector<float> data;
    std::vector<rhi::VertexBufferAttribute> a;
//    std::vector<float> vertices;
//    std::vector<float> normals;
//    std::vector<float> uvs;
//    std::vector<float> tangents;
//    std::vector<float> bitangents;
//    std::vector<float> colors;
};

struct ModelData {
    std::vector<MeshData> meshes;
};

class SceneLoader {
public:
    SceneLoader() = default;
    ~SceneLoader() = default;

    void load(const std::filesystem::path& filePath);

private:
//    std::vector
    ModelData _data;
};

} // namespace raum::framework::asset