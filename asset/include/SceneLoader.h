#pragma once
#include <filesystem>
#include "Scene.h"
#include "SceneGraph.h"
#include "RHIDevice.h"

namespace raum::framework::asset {

enum class ShaderAttribute: uint8_t {
    NONE = 0,
    POSITION = 1,
    NORMAL = 1 << 1,
    UV = 1 << 2,
    BI_TANGENT = 1 << 3,
};
OPERABLE(ShaderAttribute);

enum class TextureType : uint32_t {
    DIFFUSE,
    SPECULAR,
    AMBIENT,
    EMISSIVE,
    HEIGHT,
    NORMALS,
    SHININESS,
    OPACITY,
    DISPLACEMENT,
    LIGHTMAP,
    REFLECTION,

    BASE_COLOR,
    NORMAL_CAMERA,
    EMISSION_COLOR,
    METALNESS,
    DIFFUSE_ROUGHNESS,
    AMBIENT_OCCLUSION,


    SHEEN,
    CLEARCOAT,
    TRANSMISSION,

    COUNT,
};

struct MeshData {
    rhi::IndexType indexType{rhi::IndexType::HALF};
    uint32_t materialIndex{0};
    std::vector<uint8_t> indices;
    std::vector<float> data;
    std::vector<rhi::VertexBufferAttribute> bufferAttributes;
    std::vector<rhi::VertexAttribute> attributes;
    ShaderAttribute shaderAttrs{ShaderAttribute::NONE};
};

struct TextureData {
    uint32_t width{0};
    uint32_t height{0};
};

struct MaterialData {
    std::string name;
    std::array<rhi::ImagePtr, static_cast<uint32_t>(TextureType::COUNT)> images;
};

struct ModelData {
    std::vector<MeshData> meshes;
    std::vector<MaterialData> materials;
};

class SceneLoader {
public:
    SceneLoader() = delete;
    SceneLoader(rhi::DevicePtr device);

    ~SceneLoader() = default;

    void load(const std::filesystem::path& filePath);

private:
//    std::vector
    ModelData _data;
    rhi::DevicePtr _device;
};

} // namespace raum::framework::asset